/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir simulation functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Keep "CirMgr::randimSim()" and "CirMgr::fileSim()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

#define RN15 (size_t)(rand() & 0x7fff)

static unsigned simSize = 64;

static size_t
rn64()
{
	return (RN15<<49) ^ (RN15<<34) ^ (RN15<<19) ^ (RN15<<4) ^ (rand() & 0xf);
}

static size_t
rn32()
{
	return (RN15<<17) ^ (RN15<<2) ^ (rand() & 0x3);
}

/************************************************/
/*   Public member functions about Simulation   */
/************************************************/
void
CirMgr::randomSim()
{
	getSimLimit();
	bool unchanged;
	size_t pattern;
	unsigned count;
	for(count = 0; count < maxSimNo && maxSimFail > 0; ++count) {
		vector<size_t> simPats(_piList.size(), 0);
		for(unsigned j = 0; j < _piList.size(); ++j) {
			if(sizeof(size_t) == 8) pattern = rn64();
			else pattern = rn32();
			simPats.push_back(pattern);
			dynamic_cast<CirPiGate*>(_ids[_piList[j]])->setIn(pattern);
		}
		unchanged = true;
		if(count == 0) firstSim();
		else sim(unchanged);
		if(unchanged) --maxSimFail;
		if(_simLog != 0) writeSimLog(simPats);
	}
	correctFECfirst();
	cout << count * 64 << " patterns simulated." << endl;

	// unsigned totalgate = 0;
	// unsigned groupsize = 0;
	// for(list< IdList* >::iterator i = _FECList.begin(); i != _FECList.end(); ++i) {
	// 	totalgate += (*i)->size();
	// 	++ groupsize;
	// }
	// cout << "count = " << count << endl;
	// cout << "group size = " << _FECList.size() << " total gate = " << totalgate << endl;
}

void
CirMgr::fileSim(ifstream& patternFile)
{
	string str;
	bool unchanged;
	unsigned count = 0;
	unsigned noPatterns = 0;
	if(sizeof(size_t) == 4) simSize = 32;
	while(!patternFile.eof()) {
		bool end = false;
		vector<size_t> simPats(_piList.size(), 0);
		for(unsigned i = 0; i < simSize; ++i) {
			patternFile >> str;
			if(patternFile.eof()) {
				if(i == 0) end = true;
				break;
			}
			if(str.size() != _piList.size()) {
				cerr << "Error: Pattern(" << str << ") length(" << str.size() 
					<< ") does not match the number of inputs(" << _piList.size()
					<< ") in a circuit!!" << endl;
				clearFECList();
				FECdone = false;
				return;
			}
			for(unsigned j = 0; j < str.size(); ++j) {
				if(str[j] == '1') simPats[j] ^= (size_t(0x1)<<i);
				else if(str[j] != '0') {
					cerr << "Error: Pattern(" << str << ") contains a non-0/1 character(\'"
						<< str[j] << "\')." << endl;
					clearFECList();
					FECdone = false;
				}
			}
			++noPatterns;
		}
		if(end) break;
		for(unsigned i = 0; i < _piList.size(); ++i) {
			dynamic_cast<CirPiGate*>(_ids[_piList[i]])->setIn(simPats[i]);
		}
		if(count == 0) firstSim();
		else sim(unchanged);
		if(_simLog != 0) writeSimLog(simPats);
		++count;
	}
	correctFECfirst();
	cout << noPatterns << " patterns simulated." << endl;
}

/*************************************************/
/*   Private member functions about Simulation   */
/*************************************************/
void
CirMgr::iniFECList()
{
	clearFECList();
	_FECList.resize(1);
	_FECList.front() = new IdList;
	_FECList.front()->push_back(0); //CONST0 gate

	for(unsigned i = 0; i < _netList.size(); ++i) {
		if(_ids[_netList[i]]->getType() == AIG_GATE) {
			_FECList.front()->push_back(_netList[i] * 2);
		}
	}
	sort(_FECList.front()->begin(), _FECList.front()->end());
	FECdone = true;
}

void
CirMgr::clearFECList()
{
	if(!_FECList.empty()) {
		for(list< IdList* >::iterator i = _FECList.begin(); i != _FECList.end(); ++i) {
			delete (*i);
		}
		_FECList.clear();
	}
}

void
CirMgr::firstSim()
{
	if(!_FECList.empty()) return;
		iniFECList();
	for(unsigned i = 0; i < _netList.size(); ++i) {
		_ids[_netList[i]]->simulate();
	}
	list< IdList* >::iterator i = _FECList.begin();
	HashMap<SimHashKey, IdList* > simMap(getHashSize((*i)->size()));
	vector< IdList* > tempGroup;
	tempGroup.reserve((*i)->size());
	for(unsigned j = 0; j < (*i)->size(); ++j) {
		SimHashKey k(_ids[(*(*i))[j]/2]->_simOut);
		IdList* n;
		if(simMap.check(k, n)) {
			n->push_back((*(*i))[j]);
		}
		else if(simMap.check(~k, n)) {
			n->push_back((*(*i))[j]+1);
		}
		else {
			n = new IdList;
			n->push_back((*(*i))[j]);
			simMap.forceInsert(k, n);
			tempGroup.push_back(n);
		} 
	}
	// for(HashMap<SimHashKey, IdList* >::iterator k = simMap.begin(); k != simMap.end(); ++k) {
	// 	if((*k).second->size() == 1) delete (*k).second;
	// 	else _FECList.insert(i, (*k).second);
	// }
	for(unsigned j = 0; j < tempGroup.size(); ++j) {
		if(tempGroup[j]->size() == 1) delete tempGroup[j];
		else _FECList.insert(i, tempGroup[j]);
	}
	delete *i;
	_FECList.erase(i);
}


void
CirMgr::sim(bool& unchanged)
{
	//simulate
	for(unsigned i = 0; i < _netList.size(); ++i) {
		_ids[_netList[i]]->simulate();
	}
	//collect FEC pair
	for(list< IdList* >::iterator i = _FECList.begin(); i != _FECList.end();) {
		HashMap<SimHashKey, IdList* > simMap(getHashSize((*i)->size()));
		if((*i)->size() > 2) {
			vector< IdList* > tempGroup;
			tempGroup.reserve((*i)->size());
			for(unsigned j = 0; j < (*i)->size(); ++j) {
				bool inv = (*(*i))[j] % 2;
				SimHashKey k(_ids[(*(*i))[j]/2]->_simOut);
				if(inv) k = ~k;
				IdList* n;
				if(simMap.check(k, n)) {
					n->push_back((*(*i))[j]);
				}
				else {
					n = new IdList;
					n->push_back((*(*i))[j]);
					simMap.forceInsert(k, n);
					tempGroup.push_back(n);
				} 
			}
			// for(HashMap<SimHashKey, IdList* >::iterator k = simMap.begin(); k != simMap.end(); ++k) {
			// 	if((*k).second->size() == 1) delete (*k).second;
			// 	else _FECList.insert(i, (*k).second);
			// }
			if(tempGroup.size() != 1) {
				for(unsigned j = 0; j < tempGroup.size(); ++j) {
					if(tempGroup[j]->size() == 1) delete tempGroup[j];
					else _FECList.insert(i, tempGroup[j]);
				}
				delete *i;
				i = _FECList.erase(i);
				unchanged = false;
			}
			else {
				delete tempGroup[0];
				++i;
			}
		}
		else {
			bool inv1 = (*(*i))[0] % 2;
			bool inv2 = (*(*i))[1] % 2;
			size_t s = _ids[(*(*i))[1] / 2]->_simOut;
			if(inv1 != inv2) s = ~s;
			if(_ids[(*(*i))[0] / 2]->_simOut != s) {
				delete *i;
				i = _FECList.erase(i);
			}
			else ++i;
		}
	}
}

//make first element of each group without inv(!)
void
CirMgr::correctFECfirst()
{
	for(list< IdList* >::iterator i = _FECList.begin(); i != _FECList.end(); ++i) {
		if((*(*i))[0] % 2 == 1) {
			--(*(*i))[0];
			for(unsigned j = 1; j < (*i)->size(); ++j) {
				if((*(*i))[j] % 2 == 0) ++(*(*i))[j];
				else --(*(*i))[j];
			}
		}
	}
}

void
CirMgr::getSimLimit()
{
	if(FECdone) {
		maxSimFail = 2 + noPI/16;
		maxSimNo = int(noAIG/400);
		if(maxSimNo < 1) maxSimNo = 2;
	}
	else {
		maxSimNo = int(noAIG/50);
		if(maxSimNo < 1) maxSimNo = 2;
		maxSimFail = 3 + noPI/8;
	}
}

void
CirMgr::writeSimLog(vector<size_t>& patterns)
{
	vector<size_t> outPats;
	outPats.reserve(_poList.size());
	for(unsigned i = 0; i < _poList.size(); ++i) {
		outPats.push_back(_ids[_poList[i]]->_simOut);
	}
	for(unsigned i = 0; i < simSize; ++i) {
		for(unsigned j = 0; j < patterns.size(); ++j) {
			*_simLog << (patterns[j] & 0x1);
			patterns[j] = patterns[j] >> 1;
		}
		*_simLog << ' ';
		for(unsigned j = 0; j < outPats.size(); ++j) {
			*_simLog << (outPats[j] & 0x1);
			outPats[j] = outPats[j] >> 1;
		}
		*_simLog << '\n';
	}
	_simLog->flush();
}

void
CirMgr::removeUnusedFEC()
{
	HashSet<unsigned> unusedset(getHashSize(_netList.size()));
	unusedset.insert(0);
	for(unsigned i = 0; i < _netList.size(); ++i) {
		if(_ids[_netList[i]]->getType() == AIG_GATE) unusedset.insert(_netList[i]);
	}
	for(list< IdList* >::iterator i = _FECList.begin(); i != _FECList.end();) {
		for(unsigned j = 0; j < (*i)->size();) {
			if(!unusedset.check((*(*i))[j]/2)) {
				(*(*i))[j] = (*i)->back();
				(*i)->pop_back();
			}
			else ++j;
		}
		if((*i)->size() < 2) {
			delete(*i);
			i = _FECList.erase(i);
		}
		else {
			sort((*i)->begin(), (*i)->end());
			++i;
		}
	}
}
