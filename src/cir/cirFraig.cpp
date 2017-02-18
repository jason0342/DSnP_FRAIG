/****************************************************************************
  FileName     [ cirFraig.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir FRAIG functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2012-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
//#include "cirFraig.h"
#include "sat.h"
#include "myHashMap.h"
#include "util.h"
#include <algorithm>

using namespace std;

// TODO: Please keep "CirMgr::strash()" and "CirMgr::fraig()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

class FraigNode
{
public:
	// FraigNode(list< IdList* >::const_iterator i): _key((*(*i))[0]), _data(i) 
	// 	{ if(_key == 0) _key = (*(*_data))[1]; }
	FraigNode(list< IdList* >::iterator i, unsigned d): _key(d), _data(i) {}
	FraigNode(const FraigNode& n): _key(n._key), _data(n._data) {}
	bool operator < (const FraigNode& n) const { return (_key > n._key); } //to make minheap
	unsigned getkey() const { return _key; }
	void setkey(unsigned k) { _key = k; }
	list< IdList* >::iterator getdata() const { return _data; }
private:
	unsigned _key;
	list< IdList* >::iterator _data;
};

#define fraigCycle 400
#define maxSATCount 63
#define maxGroupSAT 22

/*******************************************/
/*   Public member functions about fraig   */
/*******************************************/
// _floatList may be changed.
// _unusedList and _undefList won't be changed
void
CirMgr::strash()
{
	HashMap<GateHashKey, CirGate*> strashMap(getHashSize(noAIG));
	CirGate* g;
	CirGate* n;
	bool changed = false;
	for(unsigned i = 0; i < _netList.size(); ++i) {
		n = _ids[_netList[i]];
		if(n->getType() == AIG_GATE) {
			GateHashKey k(n->_inList[0]->getAIGid(), n->_inList[1]->getAIGid());
			if(strashMap.check(k, g)) {
				cout << "Strashing: " << g->getId() << " merging " << n->getId() << "..." << endl;
				g->mergeGate(n);
				delGate(n);
				changed = true;
			}
			else strashMap.forceInsert(k, n);
		}
	}
	if(changed) updateDFS();
}

void
CirMgr::fraig()
{
	SatSolver solver;
	// vector<size_t> satPats(_piList.size(),0);
	vector< IdList* > eqGroups;
	vector< FraigNode > fraigHeap;
	// IdList priorityTable(maxGateNo + noPO + 1, 0);
	// HashSet<DFSHashKey> priorityHash(_netList.size());
	bool result;
	bool forceResim;
	unsigned fraigCount;
	unsigned satCount;

	/******miter checker********/ 
	// solver.initialize();
	// createPfMod(solver);
	// bool success = true;
	// for(unsigned i = 0; i < _poList.size(); ++i) {
	// 	CirGate* g = _ids[_poList[i]]->_inList[0]->gate();
	// 	bool inv = _ids[_poList[i]]->_inList[0]->isInv();
	// 	solver.assumeProperty(g->_satVar, inv);
	// 	result = solver.assumpSolve();
	// 	if(!result) success = false;
	// }
	// cout << success << endl; return;
	/****************************/

	strash();
	removeUnusedFEC();


	for(unsigned i = 0; i < _piList.size(); ++i) {
		dynamic_cast<CirPiGate*>(_ids[_piList[i]])->setIn(0);
	}

	// for(unsigned i = 0; i < _netList.size(); ++i) {
	// 	priorityTable[_netList[i]] = i;
	// }

	while(!_FECList.empty()) {

		sim(result);

		// create fraig priority queue
		for(list< IdList* >::iterator i = _FECList.begin(); i != _FECList.end(); ++i) {
			unsigned priority;
			if((*(*i))[0] == 0) priority = _ids[(*(*i))[1]/2]->_dfsOrder;
			else 
				priority = _ids[(*(*i))[0]/2]->_dfsOrder;
			fraigHeap.push_back(FraigNode(i, priority));
		}
		make_heap(fraigHeap.begin(), fraigHeap.end());

		solver.initialize();
		createPfMod(solver);

		fraigCount = 0;
		satCount = 0;
		
		while(fraigCount < fraigCycle && satCount < maxSATCount && !_FECList.empty()) {
			assert(_FECList.size() == fraigHeap.size()) ;

			forceResim = false;
			IdList* v = *(fraigHeap[0].getdata());
			if(v->size() == 2) {
				result = solvePair((*v)[0], (*v)[1], solver);
				if(result) {
					//cout << "sat " << (*v)[0] << ' ' << (*v)[1] << endl;
					getSATAssign(solver);
					++satCount;
					delete v;
				}
				else {
					eqGroups.push_back(v);
					//cout << "unsat " << (*v)[0] << ' ' << (*v)[1] << endl;
				}
				_FECList.erase(fraigHeap[0].getdata());
				pop_heap(fraigHeap.begin(), fraigHeap.end());
				fraigHeap.pop_back();
				++fraigCount;	
			}
			else {
				IdList* tempGroup = new IdList;
				unsigned groupSatCount = 0;
				for(unsigned i = 1; i < v->size();) {
					if(forceResim) {
						size_t s = _ids[(*v)[i]/2]->_simOut;
						if((*v)[i] % 2 != (*v)[0] % 2) s = ~s;
						if(_ids[(*v)[0]/2]->_simOut != s) result = true;
						else {
							result = solvePair((*v)[0], (*v)[i], solver);
							if(result) getSATAssign(solver);
							++satCount;
							++groupSatCount;
						} 
							
					}
					else result = solvePair((*v)[0], (*v)[i], solver);
					if(result) {
						//cout << "sat " << (*v)[0] << ' ' << (*v)[i] << endl;
						if(!forceResim) getSATAssign(solver);
						++groupSatCount;
						++satCount;
						tempGroup->push_back((*v)[i]);
						v->erase(v->begin() + i);
						if(v->size() > 4 && groupSatCount > 2 && !forceResim) {
							for(unsigned j = 0; j < _netList.size(); ++j) {
								_ids[_netList[j]]->simulate();
							}
							forceResim = true;
						}
						if(groupSatCount % maxGroupSAT == 0) forceResim = false;
					}
					else {
						//cout << "unsat " << (*v)[0] << ' ' << (*v)[i] << endl;
						++i;
					}
					if(satCount > maxSATCount && i < v->size() - 2) {
						tempGroup->insert(tempGroup->begin(),(*v)[0]);
						tempGroup->insert(tempGroup->end(),v->begin() + i, v->end());
						v->resize(i);
						// priorityTable[(*tempGroup)[0]/2] += noAIG/10;
						break;
					}
					++ fraigCount;
				}
				_FECList.erase(fraigHeap[0].getdata());
				pop_heap(fraigHeap.begin(), fraigHeap.end());
				fraigHeap.pop_back();
				if(v->size() < 2) delete v;
				else eqGroups.push_back(v);
				if(tempGroup->size() < 2) delete tempGroup;
				else {
					_FECList.push_front(tempGroup);
					fraigHeap.push_back(FraigNode((list< IdList* >::iterator)_FECList.begin(), _ids[(*tempGroup)[0]/2]->_dfsOrder));
					push_heap(fraigHeap.begin(), fraigHeap.end());
				}
			}
		}

		if(!eqGroups.empty()) {
			for(unsigned i = 0; i < _netList.size(); ++i) {
				//cout << _netList[i] << ' ';
				_ids[_netList[i]]->FECId = 0;
			}
			_ids[0]->FECId = 0;
			//cout << endl;

			for(unsigned i = 0; i < eqGroups.size(); ++i) {
				for(unsigned j = 0; j < eqGroups[i]->size(); ++j) {
					// if((*eqGroups[i])[j] % 2)cout << '!';
					// cout << (*eqGroups[i])[j]/2 << ' ';
					if((*eqGroups[i])[j] % 2 == 0) _ids[(*eqGroups[i])[j]/2]->FECId = i + 1;
					else _ids[(*eqGroups[i])[j]/2]->FECId = ~(i + 1);
				}
				// cout << endl;
				delete eqGroups[i];
			}

			// for(unsigned i = 0; i < _netList.size(); ++i) {
			// 	if(_ids[_netList[i]]->FECId != 0) cout << _netList[i] << ' ' << _ids[_netList[i]]->FECId << endl;
			// }

			HashMap<SimHashKey, CirGate*> strashMap(eqGroups.size());
			CirGate* g;
			CirGate* n;
			strashMap.forceInsert(_ids[0]->FECId, _ids[0]);
			for(unsigned i = 0; i < _netList.size(); ++i) {
				n = _ids[_netList[i]];
				if(n->FECId != 0 && n->getType() == AIG_GATE) {
					SimHashKey k1(n->FECId);
					SimHashKey k2(~(n->FECId));
					if(strashMap.check(k1, g)) {
						cout << "Fraig: " << g->getId() << " merging ";
						cout << n->getId() << "..." << endl;
						g->mergeGate(n, false);
						delGate(n);
					}
					else if(strashMap.check(k2, g)) {
						cout << "Fraig: " << g->getId() << " merging ";
						cout << '!';
						cout << n->getId() << "..." << endl;
						g->mergeGate(n, true);
						delGate(n);
					}
					else strashMap.forceInsert(k1, n);
				}
			}
		}
		

		// for(unsigned i = 0; i < eqGroups.size(); ++i) {
		// 	CirGate* g1 = _ids[(*eqGroups[i])[0] / 2];
		// 	for(unsigned j = 1; j < eqGroups[i]->size(); ++j) {
		// 		if(_ids[(*eqGroups[i])[j] / 2]->_dfsOrder < g1->_dfsOrder) g1 = _ids[(*eqGroups[i])[j] / 2];
		// 	}
		// 	for(unsigned j = 0; j < eqGroups[i]->size(); ++j) {
		// 		CirGate* g2 = _ids[(*eqGroups[i])[j] / 2];
		// 		if(g1 != g2) {
		// 			bool inv = ((*eqGroups[i])[j] % 2 != (*eqGroups[i])[0] % 2);
		// 			g1->mergeGate(g2, inv);
		// 			delGate(g2);
		// 			cout << "Fraig: " << g1->getId() << ' ' << g1->_dfsOrder << " merging ";
		// 			if(inv) cout << '!';
		// 			cout << g2->getId() << ' ' << g2->_dfsOrder << "..." << endl;
		// 		}	
		// 	}
		// 	// for(unsigned j = 1; j < eqGroups[i]->size(); ++j) {
		// 	// 	CirGate* g2 = _ids[(*eqGroups[i])[j] / 2];
		// 	// 	bool inv = ((*eqGroups[i])[j] % 2 != (*eqGroups[i])[0] % 2);
		// 	// 	if(g1->_dfsOrder > g2->_dfsOrder) {
		// 	// 		CirGate* temp = g1;
		// 	// 		g1 = g2;
		// 	// 		g2 = temp;
		// 	// 	}
		// 	// 	g1->mergeGate(g2, inv);
		// 	// 	delGate(g2);
		// 	// 	// for(unsigned i = 0; i < g2->_inList.size(); ++i) {
		// 	// 	// 	CirGate* n = g2->_inList[i]->gate();
		// 	// 	// 	if(n != 0 && n->getType() != CONST_GATE && n->_outList.empty()) {_unUsedList.push_back(n->getId());
		// 	// 	// }
		// 	// 	// delGate(g2);
		// 	// 	cout << "Fraig: " << g1->getId() << ' ' << g1->_dfsOrder << " merging ";
		// 	// 	if(inv) cout << '!';
		// 	// 	cout << g2->getId() << ' ' << g2->_dfsOrder << "..." << endl;
		// 	// 	//assert(g1->_dfsOrder < g2->_dfsOrder);
		// 	// }
		// 	delete eqGroups[i];
		// }
		if(!eqGroups.empty()) {
			updateDFS();
			optimize();
			strash();
		}
		eqGroups.clear();
		fraigHeap.clear();
		//updateDFS();
		removeUnusedFEC();
	}
	updateUnused();
}

/********************************************/
/*   Private member functions about fraig   */
/********************************************/
void
CirMgr::createPfMod(SatSolver& s)
{
	Var v;
	for(unsigned i = 0; i < _ids.size(); ++i) {
		if(_ids[i] != 0) {
			v = s.newVar();
			_ids[i]->_satVar = v;
		}
	}
	//create const 0 gate
	s.addAigCNF(_ids[0]->_satVar, _ids[_piList[0]]->_satVar, false,
		_ids[_piList[0]]->_satVar, true);
	for(unsigned i = 1; i < _ids.size(); ++i) {
		CirGate* g = _ids[i];
		if(g != 0 && g->getType() == AIG_GATE) {
			s.addAigCNF(g->_satVar, g->_inList[0]->getInVar(), g->_inList[0]->isInv(),
				g->_inList[1]->getInVar(), g->_inList[1]->isInv());
		}
	}
}

bool
CirMgr::solvePair(unsigned g1, unsigned g2, SatSolver& s)
{
	bool result;
	if(g1 == 0) {
		s.assumeRelease();
		s.assumeProperty(_ids[g2/2]->_satVar, !(g2 % 2));
	}
	else {
		Var resultVar = s.newVar();
		s.addXorCNF(resultVar, _ids[g1/2]->_satVar, g1 % 2, 
			_ids[g2/2]->_satVar, g2 % 2);
		s.assumeRelease();
		s.assumeProperty(resultVar, true);
	}	
	result = s.assumpSolve();
	return result;
}

void
CirMgr::getSATAssign(vector<size_t>& pats, SatSolver& s) 
{
	size_t pattern;
	for(unsigned i = 0; i < _piList.size(); ++i) {
		pattern = s.getValue(_ids[_piList[i]]->_satVar);
		pats[i] = (pats[i] << 1) ^ pattern;
	}
} 

void
CirMgr::getSATAssign(SatSolver& s)
{
	size_t pattern;
	for(unsigned i = 0; i < _piList.size(); ++i) {
		pattern = s.getValue(_ids[_piList[i]]->_satVar);
		_ids[_piList[i]]->_simOut = ((_ids[_piList[i]]->_simOut) << 1) ^ pattern;
	}
}
