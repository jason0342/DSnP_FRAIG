/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir optimization functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Please keep "CirMgr::sweep()" and "CirMgr::optimize()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

enum OpitmizationType {
	CONST0FANIN,
	CONST1FANIN,
	SAME_FANIN,
	INV_FANIN,
	NONE
};

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/**************************************************/
/*   Public member functions about optimization   */
/**************************************************/
// Remove unused gates
// DFS list should NOT be changed
// UNDEF, float and unused list may be changed
void
CirMgr::sweep()
{
	HashSet<unsigned> dfsset(getHashSize(_netList.size()));
	for(unsigned i = 0; i < _netList.size(); ++i) {
		if(_ids[_netList[i]]->getType() == AIG_GATE) dfsset.insert(_netList[i]);
	}
	for(unsigned i = 0; i < _ids.size(); ++i) {
		CirGate* current = _ids[i];
		if(current != 0 && current->getType() == AIG_GATE) {
			if(!dfsset.check(i)) {
				CirGate* previous;
				for(unsigned i = 0; i < current->_inList.size(); ++i) {
					if(!current->_inList[i]->isUndef()) previous = current->_inList[i]->gate();
					else {
						cout << "Sweeping: UNDEF(" << current->_inList[i]->getAIGid() / 2
							<< ") removed..." << endl;
						continue;
					}
					if(previous != 0) {
						previous->delOut(current);
					}
				}
				cout << "Sweeping: " << current->getTypeStr() << '(' << current->_id 
					<< ") removed..." << endl;
				delGate(current);
			}
		}
	}
}

// Recursively simplifying from POs;
// _dfsList needs to be reconstructed afterwards
// UNDEF gates may be delete if its fanout becomes empty...
void
CirMgr::optimize()
{
	bool changed = false;
	OpitmizationType optType;
	for(unsigned i = 0; i < _netList.size(); ++i) {
		optType = NONE;
		CirGate* g = _ids[_netList[i]];
		unsigned j = 0;
		if(g->_inList.size() == 2) {
			if(*(g->_inList[0]) == *(g->_inList[1])) optType = SAME_FANIN;
			else if(*(g->_inList[0]) != *(g->_inList[1])) optType = INV_FANIN;
			else for(j = 0; j < g->_inList.size(); ++j) {
				if(g->_inList[j]->isCONST0()) {
					optType = CONST0FANIN;
					break;
				}
				else if(g->_inList[j]->isCONST1()) {
					optType = CONST1FANIN;
					break;
				}
			}
			switch(optType) {
				case CONST0FANIN:
				case INV_FANIN:
					optDelGate(g, CirGateIn(_ids[0], 0));
					break;
				case CONST1FANIN:
					optDelGate(g, *(g->_inList[!j]));
					break;
				case SAME_FANIN:
					optDelGate(g, *(g->_inList[0]));
				case NONE:
					break;
			}
		}
		if(optType != NONE) changed = true;
	}
	if(changed) updateDFS();
}

/***************************************************/
/*   Private member functions about optimization   */
/***************************************************/
void
CirMgr::sweepGate(CirGate* current)
{
	current->_outList.clear();
	if(current->getType() != AIG_GATE) return;
	CirGate* previous;
	for(unsigned i = 0; i < current->_inList.size(); ++i) {
		if(!current->_inList[i]->isUndef()) previous = current->_inList[i]->gate();
		else {
			cout << "Sweeping: UNDEF(" << current->_inList[i]->getAIGid() / 2
				<< ") removed..." << endl;
			continue;
		}
		if(previous->_outList.size() == 1) sweepGate(previous);
		else previous->delOut(current);
	}
	cout << "Sweeping: " << current->getTypeStr() << '(' << current->_id 
		<< ") removed..." << endl;
	delGate(current);
}

void
CirMgr::optDelGate(const CirGate* g, const CirGateIn& in)
{
	cout << "Simplifying: ";
	in.print();
	cout << " merging " << g->getId() << "..." << endl;

	CirGate* n;
	for(unsigned i = 0; i < g->_outList.size(); ++i) {
		n = g->_outList[i];
		for(unsigned j = 0; j < n->_inList.size(); ++j) {
			if(n->_inList[j]->gate() == g) n->_inList[j]->setGateInv(in);
		}
	}
	n = in.gate();
	if(!in.isUndef()) {
		for(unsigned i = 0; i < g->_outList.size(); ++i) {
			n->_outList.push_back(g->_outList[i]);
		}
	}
	for(unsigned i = 0; i < g->_inList.size(); ++i) {
		n = g->_inList[i]->gate();
		n->delOut(g);
		//if(n != 0 && n->getType() != CONST_GATE && n->_outList.empty()) _unUsedList.push_back(n->getId());
	}
	delGate(g);
}
