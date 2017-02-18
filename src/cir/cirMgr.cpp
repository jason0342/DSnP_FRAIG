/****************************************************************************
  FileName     [ cirMgr.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir manager functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cstdio>
#include <ctype.h>
#include <cassert>
#include <cstring>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Implement memeber functions for class CirMgr

/*******************************/
/*   Global variable and enum  */
/*******************************/
CirMgr* cirMgr = 0;

enum CirParseError {
   EXTRA_SPACE,
   MISSING_SPACE,
   ILLEGAL_WSPACE,
   ILLEGAL_NUM,
   ILLEGAL_IDENTIFIER,
   ILLEGAL_SYMBOL_TYPE,
   ILLEGAL_SYMBOL_NAME,
   MISSING_NUM,
   MISSING_IDENTIFIER,
   MISSING_NEWLINE,
   MISSING_DEF,
   CANNOT_INVERTED,
   MAX_LIT_ID,
   REDEF_GATE,
   REDEF_SYMBOLIC_NAME,
   REDEF_CONST,
   NUM_TOO_SMALL,
   NUM_TOO_BIG,

   DUMMY_END
};

/**************************************/
/*   Static varaibles and functions   */
/**************************************/
// static unsigned lineNo = 0;  // in printint, lineNo needs to ++
// static unsigned colNo  = 0;  // in printing, colNo needs to ++
// static char buf[1024];
// static string errMsg;
// static int errInt;
// static CirGate *errGate;

// static bool
// parseError(CirParseError err)
// {
//    switch (err) {
//       case EXTRA_SPACE:
//          cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
//               << ": Extra space character is detected!!" << endl;
//          break;
//       case MISSING_SPACE:
//          cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
//               << ": Missing space character!!" << endl;
//          break;
//       case ILLEGAL_WSPACE: // for non-space white space character
//          cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
//               << ": Illegal white space char(" << errInt
//               << ") is detected!!" << endl;
//          break;
//       case ILLEGAL_NUM:
//          cerr << "[ERROR] Line " << lineNo+1 << ": Illegal "
//               << errMsg << "!!" << endl;
//          break;
//       case ILLEGAL_IDENTIFIER:
//          cerr << "[ERROR] Line " << lineNo+1 << ": Illegal identifier \""
//               << errMsg << "\"!!" << endl;
//          break;
//       case ILLEGAL_SYMBOL_TYPE:
//          cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
//               << ": Illegal symbol type (" << errMsg << ")!!" << endl;
//          break;
//       case ILLEGAL_SYMBOL_NAME:
//          cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
//               << ": Symbolic name contains un-printable char(" << errInt
//               << ")!!" << endl;
//          break;
//       case MISSING_NUM:
//          cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
//               << ": Missing " << errMsg << "!!" << endl;
//          break;
//       case MISSING_IDENTIFIER:
//          cerr << "[ERROR] Line " << lineNo+1 << ": Missing \""
//               << errMsg << "\"!!" << endl;
//          break;
//       case MISSING_NEWLINE:
//          cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
//               << ": A new line is expected here!!" << endl;
//          break;
//       case MISSING_DEF:
//          cerr << "[ERROR] Line " << lineNo+1 << ": Missing " << errMsg
//               << " definition!!" << endl;
//          break;
//       case CANNOT_INVERTED:
//          cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
//               << ": " << errMsg << " " << errInt << "(" << errInt/2
//               << ") cannot be inverted!!" << endl;
//          break;
//       case MAX_LIT_ID:
//          cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
//               << ": Literal \"" << errInt << "\" exceeds maximum valid ID!!"
//               << endl;
//          break;
//       case REDEF_GATE:
//          cerr << "[ERROR] Line " << lineNo+1 << ": Literal \"" << errInt
//               << "\" is redefined, previously defined as "
//               << errGate->getTypeStr() << " in line " << errGate->getLineNo()
//               << "!!" << endl;
//          break;
//       case REDEF_SYMBOLIC_NAME:
//          cerr << "[ERROR] Line " << lineNo+1 << ": Symbolic name for \""
//               << errMsg << errInt << "\" is redefined!!" << endl;
//          break;
//       case REDEF_CONST:
//          cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
//               << ": Cannot redefine const (" << errInt << ")!!" << endl;
//          break;
//       case NUM_TOO_SMALL:
//          cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
//               << " is too small (" << errInt << ")!!" << endl;
//          break;
//       case NUM_TOO_BIG:
//          cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
//               << " is too big (" << errInt << ")!!" << endl;
//          break;
//       default: break;
//    }
//    return false;
// }

/**************************************************************/
/*   class CirMgr member functions for circuit construction   */
/**************************************************************/
bool
CirMgr::readCircuit(const string& fileName)
{
	fstream file;
	string line;
	string token;
	vector<string> options;
	unsigned currentLine = 1;
	FECdone = false;

	file.open(fileName.c_str(), ios::in);
	if(!file) {
		cerr << "File does not exist!" << endl;
		return false;
	}
	getline(file, line);
	size_t n = myStrGetTok(line, token);
	while(!token.empty()) {
		options.push_back(token);
		n = myStrGetTok(line, token, n);
	}
	if(options.size() == 6) {
		if(options[0] == "aag") {
			int temp;
			if(myStr2Int(options[1], temp) && temp >= 0) maxGateNo = temp;
			if(myStr2Int(options[2], temp) && temp >= 0) noPI = temp;
			if(myStr2Int(options[3], temp) && temp >= 0) noLA = temp;
			if(myStr2Int(options[4], temp) && temp >= 0) noPO = temp;
			if(myStr2Int(options[5], temp) && temp >= 0) noAIG = temp;
			_ids.resize(maxGateNo + noPO + 1, 0);
		}
	}

	int id;
	vector<vector<unsigned> > linkID;
	linkID.resize(maxGateNo + noPO + 1);
	
	while(getline(file, line)) {
		++currentLine;
		options.clear();
		n = myStrGetTok(line, token);

		if(!token.empty()) {
			if(myStr2Int(token, id)) {
				while(!token.empty()) {
					options.push_back(token);
					n = myStrGetTok(line, token, n);
				}
				if(options.size() == 1) {
					//POgate
					if(currentLine > noPI + 1) {
						unsigned thisID = maxGateNo + _poList.size() + 1;
						linkID[thisID].push_back(id);
						CirGate* po = new CirPoGate(thisID, currentLine);
						_poList.push_back(thisID);
						_ids[thisID] = po;
					}
					//PIgate
					else {
						if(id % 2 == 0 && id <= 2 * maxGateNo) {
							id /= 2;
							CirGate* pin = new CirPiGate(id, currentLine);
							_piList.push_back(id);
							_ids[id] = pin;
						}
					}
				}
				//AIGgate
				else if(options.size() == 3){
					if(id % 2 == 0 && id <= 2 * maxGateNo) {
						id /= 2;
						CirGate* aig = new CirAigGate(id, currentLine);
						_ids[id] = aig;
						int aigIn;
						if(myStr2Int(options[1], aigIn)) linkID[id].push_back(aigIn);
						if(myStr2Int(options[2], aigIn)) linkID[id].push_back(aigIn);
						// _FECList.front()->push_back(id*2);
					}
				}
			}
			else if(token[0] == 'i') {
				token.erase(token.begin());
				if(myStr2Int(token, id)) {
					if(id < _piList.size() && _ids[_piList[id]] != 0) {
						((CirPiGate*)_ids[_piList[id]])->_name = line.substr(++n);
					}
				}
			}
			else if(token[0] == 'o') {
				token.erase(token.begin());
				if(myStr2Int(token, id)) {
					if(id < _poList.size() && _ids[_poList[id]] != 0) {
						((CirPoGate*)_ids[_poList[id]])->_name = line.substr(++n);
					}
				}
			}
			else if(token[0] == 'c') break;
		}
	}

	//link the gates
	_ids[0] = new CirConstGate();
	for(unsigned i = 0; i < linkID.size(); ++i) {
		//POgate
		if(linkID[i].size() == 1) {
			id = linkID[i][0];
			if(id == 0 || id == 1) {
				((CirPoGate*)_ids[i])->_inList[0] = new CirGateIn(_ids[0], id);
				_ids[0]->_outList.push_back(_ids[i]);
			}
			else if(id <= 2 * maxGateNo + 1) {
				size_t inv = id % 2;
				id /= 2;
				if(_ids[id] != 0) {
					((CirPoGate*)_ids[i])->_inList[0] = new CirGateIn(_ids[id], inv);
					_ids[id]->_outList.push_back(_ids[i]);
				}
				else {
					_floatingList.push_back(i);
					((CirPoGate*)_ids[i])->_inList[0] = new CirGateIn();
					((CirPoGate*)_ids[i])->_inList[0]->setUndefID(id, inv);
				}
			}
			assert(((CirPoGate*)_ids[i])->_inList[0] != 0);
		}
		//AIG
		else if(linkID[i].size() == 2) {
			bool floating = false;
			for(unsigned j = 0; j < linkID[i].size(); ++j) {
				id = linkID[i][j];
				if(id == 0 || id == 1) {
					((CirAigGate*)_ids[i])->_inList[j] = new CirGateIn(_ids[0], id);
					_ids[0]->_outList.push_back(_ids[i]);
				}
				else if(id <= 2 * maxGateNo + 1) {
					size_t inv = id % 2;
					id /= 2;
					if(_ids[id] != 0) {
						((CirAigGate*)_ids[i])->_inList[j] = new CirGateIn(_ids[id], inv);
						_ids[id]->_outList.push_back(_ids[i]);
					}
					else {
						((CirAigGate*)_ids[i])->_inList[j] = new CirGateIn();
						((CirAigGate*)_ids[i])->_inList[j]->setUndefID(id, inv);
						floating = true;
					}
				}
				assert(((CirAigGate*)_ids[i])->_inList[j] != 0);
			}
			if(floating) _floatingList.push_back(i);
		}
	}

	//scan for unused gates
	for(unsigned i = 1; i <= maxGateNo; ++i) {
		if(_ids[i] != 0 && _ids[i]->_outList.empty()) _unUsedList.push_back(i);
	}

	//build netList
	for(unsigned i = 0; i < _poList.size(); ++i) {
		traverse(_ids[_poList[i]]);
	}

	//set dfsOrder
	for(unsigned i = 0; i < _netList.size(); ++i) {
		_ids[_netList[i]]->_dfsOrder = i + 1;
	}
	_ids[0]->_dfsOrder = 0;

	return true;
}

/**********************************************************/
/*   class CirMgr member functions for circuit printing   */
/**********************************************************/
/*********************
Circuit Statistics
==================
  PI          20
  PO          12
  AIG        130
------------------
  Total      162
*********************/
void
CirMgr::printSummary() const
{
	cout << "\nCircuit Statistics\n==================\n";
	cout << "  PI" << setw(12) << noPI << '\n';
	cout << "  PO" << setw(12) << noPO << '\n';
	cout << "  AIG" << setw(11) << noAIG << '\n';
	cout << "------------------\n";
	cout << "  Total" << setw(9) << noPI + noPO + noAIG << endl;
}

void
CirMgr::printNetlist() const
{
	cout << '\n';
	for(unsigned i = 0; i < _netList.size(); ++i) {
		cout << '[' << i << "] ";
		_ids[_netList[i]]->printGate();
	}

}

void
CirMgr::printPIs() const
{
	cout << "PIs of the circuit:";
	for(unsigned i = 0; i < _piList.size(); ++i) {
		cout << ' ' << _piList[i];
	}
	cout << endl;
}

void
CirMgr::printPOs() const
{
	cout << "POs of the circuit:";
	for(unsigned i = 0; i < _poList.size(); ++i) {
		cout << ' ' << _poList[i];
	}
	cout << endl;
}

void
CirMgr::printFloatGates()
{
	updateFloat();
	updateUnused();
	sort(_floatingList.begin(), _floatingList.end());
	sort(_unUsedList.begin(), _unUsedList.end());

	if(_floatingList.size() != 0) {
		cout << "Gates with floating fanin(s):";
		for(unsigned i = 0; i < _floatingList.size(); ++i) {
			cout << ' ' << _floatingList[i];
		}
		cout << endl;
	}
	if(_unUsedList.size() != 0) {
		cout << "Gates defined but not used  :";
		for(unsigned i = 0; i < _unUsedList.size(); ++i) {
			cout << ' ' << _unUsedList[i];
		}
		cout << endl;
	}
}

void
CirMgr::printFECPairs() const
{
	unsigned count = 0;
	bool inv;
	for(list< IdList* >::const_iterator i = _FECList.begin(); i != _FECList.end(); ++i) {
		cout << '[' << count << ']';
		for(unsigned j = 0; j < (*i)->size(); ++j) {
			inv = (*(*i))[j] % 2;
			cout << ' ';
			if(inv) cout << '!';
			cout << (*(*i))[j] / 2;
		}
		cout << endl;
		++count;
	}
}

void
CirMgr::writeAag(ostream& outfile) const
{
	IdList netAIG;
	for(unsigned i = 0; i < _netList.size(); ++i) {
		if(_ids[_netList[i]]->getType() == AIG_GATE) netAIG.push_back(_netList[i]);
	}
	outfile << "aag " << maxGateNo << ' ' << noPI << ' ' 
		<< noLA << ' ' << noPO << ' ' << netAIG.size() << '\n';
	for(unsigned i = 0; i < _piList.size(); ++i) {
		outfile <<  _ids[_piList[i]]->_id * 2 << '\n';
	}
	for(unsigned i = 0; i < _poList.size(); ++i) {
		CirGateIn* in = _ids[_poList[i]]->_inList[0];
		outfile << in->getAIGid() << '\n';
	}
	for(unsigned i = 0; i < netAIG.size(); ++i) {
		outfile << _ids[netAIG[i]]->_id * 2;
		for(unsigned j = 0; j < _ids[netAIG[i]]->_inList.size(); ++j) {
			outfile << ' ' << _ids[netAIG[i]]->_inList[j]->getAIGid();
 		}
		outfile << '\n';
	}
	for(unsigned i = 0; i < _piList.size(); ++i) {
		if(!_ids[_piList[i]]->_name.empty()) {
			outfile << 'i' << i << ' ' << _ids[_piList[i]]->_name << '\n';
		}
	}
	for(unsigned i = 0; i < _poList.size(); ++i) {
		if(!_ids[_poList[i]]->_name.empty()) {
			outfile << 'o' << i << ' ' << _ids[_poList[i]]->_name << '\n';
		}
	}
	outfile << 'c' << endl;
}

void
CirMgr::writeGate(ostream& outfile, CirGate *g) const
{
	IdList nl;
	IdList pilist;
	IdList aiglist;
	++CirGate::_globalFlag;
	traverse(g, nl);
	unsigned maxId = 0;
	unsigned nPI = 0;
	unsigned nAIG = 0;
	for(unsigned i = 0; i < nl.size(); ++i) {
		switch(_ids[nl[i]]->getType()) {
			case PI_GATE: ++nPI; pilist.push_back(nl[i]); break;
			case AIG_GATE: ++nAIG; aiglist.push_back(nl[i]); break;
			default: break;
		}
		if(nl[i] > maxId) maxId = nl[i];
	}
	outfile << "aag " << maxId << ' ' << nPI << " 0 1 " << nAIG << '\n';
	for(unsigned i = 0; i < pilist.size(); ++i) {
		outfile <<  _ids[pilist[i]]->_id * 2 << '\n';
	}
	outfile <<  _ids[nl.back()]->_id * 2 << '\n';
	for(unsigned i = 0; i < aiglist.size(); ++i) {
		outfile << _ids[aiglist[i]]->_id * 2;
		for(unsigned j = 0; j < _ids[aiglist[i]]->_inList.size(); ++j) {
			outfile << ' ' << _ids[aiglist[i]]->_inList[j]->getAIGid();
 		}
 		outfile << '\n';
	}
	for(unsigned i = 0; i < pilist.size(); ++i) {
		if(!_ids[_piList[i]]->_name.empty()) {
			outfile << 'i' << i << ' ' << _ids[_piList[i]]->_name << '\n';
		}
	}
	outfile << 'o' << "0 " << g->getId() << '\n';
	outfile << 'c' << endl;
}

void
CirMgr::traverse(CirGate* g)
{
	CirGateIn* ingate;
	//g->setFlag();
	for(unsigned i = 0; i < g->_inList.size(); ++i) {
		ingate = g->_inList[i];
		assert(ingate != 0);
		if(!ingate->isUndef() && !ingate->gate()->flag()) traverse(ingate->gate()); 
	}
	g->setFlag();
	_netList.push_back(g->_id);
}

void
CirMgr::traverse(CirGate* g, IdList& nl) const
{
	CirGateIn* ingate;
	for(unsigned i = 0; i < g->_inList.size(); ++i) {
		ingate = g->_inList[i];
		assert(ingate != 0);
		if(!ingate->isUndef() && !ingate->gate()->flag()) traverse(ingate->gate(), nl); 
	}
	g->setFlag();
	nl.push_back(g->_id);
}


CirMgr::~CirMgr() {
	for(unsigned i = 0; i < _ids.size(); ++i) {
		if(_ids[i] != 0) delete _ids[i];
	}
	clearFECList();
}

void
CirMgr::updateFloat()
{
	_floatingList.clear();
	for(unsigned i = 1; i < _ids.size(); ++i) {
		if(_ids[i] != 0) {
			for(unsigned j = 0; j < _ids[i]->_inList.size(); ++j) {
				if(_ids[i]->_inList[j]->isUndef()) {
					_floatingList.push_back(i);
					break;
				}
			}
		}
	}
}

void
CirMgr::updateUnused()
{
	_unUsedList.clear();
	for(unsigned i = 1; i < _ids.size(); ++i) {
		if(_ids[i] != 0 && _ids[i]-> getType() != PO_GATE  && _ids[i]->_outList.empty()) 
			_unUsedList.push_back(i);
	}
}

void
CirMgr::updateDFS()
{
	_netList.clear();
	++CirGate::_globalFlag;
	for(unsigned i = 0; i < _poList.size(); ++i) {
		traverse(_ids[_poList[i]]);
	}
	// for(unsigned i = 0; i < _netList.size(); ++i) {
	// 	_ids[_netList[i]]->_dfsOrder = i + 1;
	// }
	// _ids[0]->_dfsOrder = 0;
}

void
CirMgr::updateAll()
{
	updateDFS();
	updateUnused();
	updateFloat();
}

void
CirMgr::delGate(const CirGate* g) 
{
	unsigned id = g->getId();
	for(unsigned i = 0; i < _floatingList.size(); ++i) {
		if(_floatingList[i] == id) {
			_floatingList.erase(_floatingList.begin() + i);
			break;
		}
	}
	delete g;
	_ids[id] = 0;
	--noAIG;
}

void
CirMgr::getFECs(unsigned id, IdList& v) const
{
	for(list< IdList* >::const_iterator i = _FECList.begin(); i != _FECList.end(); ++i) {
		for(unsigned j = 0; j < (*i)->size(); ++j) {
			bool inv = false;
			if((*(*i))[j]/2 == id) {
				if((*(*i))[j] % 2 == 1) inv = true;
				for(unsigned k = 0; k < (*i)->size(); ++k) {
					if(k == j) continue;
					if(inv) {
						if((*(*i))[k] % 2 == 0) v.push_back((*(*i))[k] + 1);
						else v.push_back((*(*i))[k] - 1);
					}
					else v.push_back((*(*i))[k]);
				}
				return;
			}
		}
	}
}

