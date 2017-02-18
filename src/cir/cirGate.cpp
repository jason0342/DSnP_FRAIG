/****************************************************************************
  FileName     [ cirGate.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define class CirAigGate member functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdarg.h>
#include <cassert>
#include "cirGate.h"
#include "cirMgr.h"
#include "util.h"

using namespace std;

// TODO: Keep "CirGate::reportGate()", "CirGate::reportFanin()" and
//       "CirGate::reportFanout()" for cir cmds. Feel free to define
//       your own variables and functions.

extern CirMgr *cirMgr;

size_t CirGate::_globalFlag = 1;

/**************************************/
/*   class CirGate member functions   */
/**************************************/
void
CirGate::reportGate() const
{
	stringstream ss;
	IdList FECs;
	cout << "==================================================" << endl;
	ss << "= "<<getTypeStr() << '(' << _id << ")";
	if(!_name.empty()) ss << '\"' << _name << '\"';
	ss <<	", line" << _lineNo;
	cout << setw(49) << left << ss.str() << '=' << endl;
	ss.str("");
	if(getType() == AIG_GATE || getType() == CONST_GATE) cirMgr->getFECs(_id, FECs);
	ss << "= FECs: ";
	for(unsigned i = 0; i < FECs.size(); ++i) {
		if(FECs[i] % 2 == 1) ss << '!';
		ss << FECs[i] / 2 << ' ';
	}
	cout << setw(49) << left << ss.str() << '=' << endl;
	ss.str("");
	ss << "= Value: ";
	for(unsigned i = 0; i < 32; ++i) {
		ss << ((_simOut >> i) & 0x1);
		if(i % 4 == 3 && i != 31) ss << '_';
	}
	cout << setw(49) << left << ss.str() << '=' << endl;
	cout << "==================================================" << endl;
}

void
CirGate::reportFanin(int level) const
{
	assert (level >= 0);

	static unsigned count = 0;
	if(count == 0) ++_globalFlag;

	cout << getTypeStr() << ' ' << _id; 
	if(flag() && level != 0) {
		cout << " (*)";
		--count;
		return;
	}
	if(level > 0) {
		for(unsigned i = 0; i < _inList.size(); ++i) {
			cout << '\n' << setw(2 * count + 2) << "";
			if(_inList[i]->isInv()) cout << '!';
			if(_inList[i]->isUndef()) cout << "UNEDF " << _inList[i]->_undefID;
			else {
				++count;
				_inList[i]->gate()->reportFanin(level - 1);
			}
		}
		if(_inList.size() != 0) setFlag();
	}
	if(count != 0) --count;
	else cout << endl;
}

void
CirGate::reportFanout(int level) const
{
	assert (level >= 0);

	static unsigned count = 0;
	if(count == 0) ++_globalFlag;

	cout << getTypeStr() << ' ' << _id; 
	if(flag() && level != 0) {
		cout << " (*)";
		--count;
		return;
	}
	if(level > 0) {
		for(unsigned i = 0; i < _outList.size(); ++i) {
			cout << '\n' << setw(2 * count + 2) << "";
			if(outIsInv(i)) cout << '!';
			++count;
			_outList[i]->reportFanout(level - 1);
		}
		if(_outList.size() != 0) setFlag();
	}
	if(count != 0) --count;
	else cout << endl;
}

bool
CirGate::outIsInv(unsigned n) const
{
	CirGate* g = _outList[n];
	for(unsigned i = 0; i < g->_inList.size(); ++i) {
		if(g->_inList[i]->gate() == this) return g->_inList[i]->isInv();
	}
	return false; //function should not reach this line
}

void
CirGateIn::print() const
{
	if(isUndef()) {
		cout << '*';
		if(isInv()) cout << '!';
		cout <<  _undefID;
	}
	else {
		if(isInv()) cout << '!';
		cout << gate()->getId();
	}
}

unsigned
CirGateIn::getAIGid() const
{
	if(isUndef()) return _undefID * 2 + isInv();
	else return gate()->_id * 2 + isInv();
}

void
CirAigGate::printGate() const
{
	cout << "AIG " << _id;
	for(unsigned i = 0; i < _inList.size(); ++i) {
		cout << ' ';
		_inList[i]->print();
	}
	cout << endl;
}

void
CirGate::mergeGate(CirGate* g, bool inv)
{
	CirGate* n;
	// cout << getId() << ' ' << _dfsOrder << " merging " << g->getId() << ' ' << g->_dfsOrder << endl;
	// assert(_dfsOrder < g->_dfsOrder);
	for(unsigned i = 0; i < g->_outList.size(); ++i) {
		n = g->_outList[i];
		for(unsigned j = 0; j < n->_inList.size(); ++j) {
			if(n->_inList[j]->gate() == g) n->_inList[j]->setGateInv(CirGateIn(this, inv));
		}
		_outList.push_back(g->_outList[i]);
	}
	for(unsigned i = 0; i < g->_inList.size(); ++i) {
		if(!g->_inList[i]->isUndef()) g->_inList[i]->gate()->delOut(g);
	}
}

void
CirPiGate::printGate() const
{
	cout << "PI  " << _id;
	if(!_name.empty()) cout << " (" << _name << ')';
	cout << endl;
}

void
CirPoGate::printGate() const
{
	cout << "PO  " << _id;
	cout << ' ';
	_inList[0]->print();
	if(!_name.empty()) cout << " (" << _name << ')';
	cout << endl;
}

void
CirGate::delOut(const CirGate* g)
{
	for(unsigned i = 0; i < _outList.size();) {
		if(_outList[i] == g) {
			_outList[i] = _outList.back();
			_outList.pop_back();
		}
		else ++i;
	}
}

void
CirAigGate::simulate()
{
	_simOut = _inList[0]->getSimOut() & _inList[1]->getSimOut();
}

void
CirPoGate::simulate()
{
	_simOut = _inList[0]->getSimOut();
}
