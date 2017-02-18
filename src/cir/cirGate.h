/****************************************************************************
  FileName     [ cirGate.h ]
  PackageName  [ cir ]
  Synopsis     [ Define basic gate data structures ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_GATE_H
#define CIR_GATE_H

#include <string>
#include <vector>
#include <iostream>
#include "cirDef.h"
#include "sat.h"

using namespace std;

// TODO: Feel free to define your own classes, variables, or functions.

class CirGate;
class CirGateIn;

//------------------------------------------------------------------------
//   Define classes
//------------------------------------------------------------------------
class CirGate
{
public:
	friend class CirMgr;
	friend class CirGateIn;

	CirGate(unsigned id, unsigned lineNo, unsigned inSize): 
		_flag(0), _id(id), _lineNo(lineNo), _inList(inSize, 0), _dfsOrder(0), _simOut(0) {}
	virtual ~CirGate() {}

	// Basic access methods
	virtual string getTypeStr() const = 0;
	virtual GateType getType() const = 0;
	unsigned getLineNo() const { return _lineNo; }

	// Printing functions
	virtual void printGate() const = 0;
	void reportGate() const;
	void reportFanin(int level) const;
	void reportFanout(int level) const;

	bool flag() const { return (_flag == _globalFlag); }
	void setFlag() const { _flag = _globalFlag; }
	unsigned getId() const { return _id; }
	bool outIsInv(unsigned) const;
	bool isAig() const{if (getType() == AIG_GATE) return true; return false; }

	virtual void simulate() = 0;
	void delOut(const CirGate*);
	void mergeGate(CirGate*, bool inv = false);

protected:
	mutable size_t _flag;
	static size_t _globalFlag;
	unsigned _id;
	unsigned _lineNo;
	string _name;
	vector<CirGate*> _outList;
	vector<CirGateIn*> _inList;
	unsigned _dfsOrder;
	size_t _simOut;
	unsigned FECId;
	Var _satVar;
};

class CirGateIn
{
public:
	#define NEG 0x1
	friend class CirGate;
	friend class CirMgr;

	// CirGateIn(CirGate* g = 0, size_t phase = 0): _gate(size_t(g) + phase), _undefID(0) {}
	CirGateIn(CirGate* g = 0, size_t phase = 0): inv(phase), _gate(g), _undefID(0) {}
	~CirGateIn() {}

	// bool operator == (const CirGateIn& g) const { return (_undefID == g._undefID && _gate == g._gate ); }
	bool operator == (const CirGateIn& g) const { return (_undefID == g._undefID && _gate == g._gate && inv == g.inv ); }
	bool operator != (const CirGateIn& g) const { 
		return (_undefID == g._undefID && gate() == g.gate() && isInv() != g.isInv()); 
	}
	void operator = (const CirGateIn& g) { _gate = g._gate; _undefID = g._undefID; inv = g.inv; }
	// void operator = (const CirGateIn& g) { _gate = g._gate; _undefID = g._undefID; }
	// CirGate* gate() const { return (CirGate*)(_gate & ~size_t(NEG)); }
	// void setGate(CirGate* g, size_t phase) { _gate = size_t(g) + phase; }
	CirGate* gate() const { return _gate; }
	Var getInVar() const { return _gate->_satVar; }
	void setGate(CirGate* g, bool phase) { _gate = g; inv = phase; }
	void setGate(const CirGateIn& g) { *this = g; }
	void setGateInv(const CirGateIn& g) { _gate = g._gate; _undefID = g._undefID; inv = (inv != g.inv); }
	// void setUndefID(unsigned id, size_t inv) {_undefID = id; _gate = inv; }
	// bool isInv() const { return (_gate & NEG); }
	void setUndefID(unsigned id, size_t phase) { _undefID = id; inv = phase; }
	bool isInv() const { return inv; }
	bool isUndef() const { return _undefID; }
	bool isCONST0() const { if(_undefID == 0) return(gate()->_id == 0 && !isInv()); return false; }
	bool isCONST1() const { if(_undefID == 0) return(gate()->_id == 0 && isInv()); return false; }
	void print() const;
	unsigned getAIGid() const;
	size_t getSimOut() const { return inv ? ~(gate()->_simOut) : gate()->_simOut; } 

private: 
	bool inv;
	CirGate* _gate;
	// size_t _gate;
	unsigned _undefID;
};

class CirAigGate: public CirGate
{
public:
	friend class CirMgr;
	CirAigGate(unsigned id, unsigned lineNo): CirGate(id, lineNo, 2) {}
	~CirAigGate() {
		for(unsigned i = 0; i < _inList.size(); ++i) {
			if(_inList[i] != 0) delete _inList[i];
		}
	}

private:
	string getTypeStr() const { return "AIG"; }
	GateType getType() const { return AIG_GATE; }
	void printGate() const;
	void simulate();
};

class CirPiGate: public CirGate
{
public:
	friend class CirMgr;
	CirPiGate(unsigned id, unsigned lineNo): CirGate(id, lineNo, 0) {}
	~CirPiGate() {}

private:
	string getTypeStr() const { return "PI"; }
	GateType getType() const { return PI_GATE; }
	void printGate() const;
	void setIn(size_t s) { _simOut = s; }
	void simulate() {}

};

class CirPoGate: public CirGate
{
public:
	friend class CirMgr;
	CirPoGate(unsigned id, unsigned lineNo): CirGate(id, lineNo, 1) {}
	~CirPoGate() { delete _inList[0]; }

private:
	string getTypeStr() const { return "PO"; }
	GateType getType() const { return PO_GATE; }
	void printGate() const;
	void simulate();

};

class CirConstGate: public CirGate
{
public:
	CirConstGate():CirGate(0, 0, 0) {}

	string getTypeStr() const { return "CONST"; }
	GateType getType() const { return CONST_GATE; }
	void printGate() const { cout << "CONST0" << endl; }
	void simulate() {}
};


#endif // CIR_GATE_H
