/****************************************************************************
  FileName     [ cirMgr.h ]
  PackageName  [ cir ]
  Synopsis     [ Define circuit manager ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_MGR_H
#define CIR_MGR_H

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <list>

using namespace std;

// TODO: Feel free to define your own classes, variables, or functions.

#include "cirDef.h"

extern CirMgr *cirMgr;

class CirMgr
{
public:
	CirMgr() {}
	~CirMgr(); 

	// Access functions
	// return '0' if "gid" corresponds to an undefined gate.
	CirGate* getGate(unsigned gid) const { if(gid < _ids.size()) return _ids[gid]; else return 0; }
	void getFECs(unsigned, IdList&) const;

	// Member functions about circuit construction
	bool readCircuit(const string&);

	// Member functions about circuit optimization
	void sweep();
	void optimize();

	// Member functions about simulation
	void randomSim();
	void fileSim(ifstream&);
	void setSimLog(ofstream *logFile) { _simLog = logFile; }

	// Member functions about fraig
	void strash();
	void printFEC() const;
	void fraig();

	// Member functions about circuit reporting
	void printSummary() const;
	void printNetlist() const;
	void printPIs() const;
	void printPOs() const;
	void printFloatGates();
	void printFECPairs() const;
	void writeAag(ostream&) const;
	void writeGate(ostream&, CirGate*) const;

private:
	ofstream           *_simLog;
	IdList _piList;
	IdList _poList;
	IdList _floatingList;
	IdList _unUsedList;
	IdList _netList;
	GateList _ids;
	list< IdList* > _FECList;
	size_t noPI;
	size_t noPO;
	size_t noLA;
	size_t noAIG;
	size_t maxGateNo;
	bool FECdone;

	unsigned maxSimFail;
	unsigned maxSimNo;

	//helper function
	void traverse(CirGate*);
	void traverse(CirGate*, IdList&) const;
	void sweepGate(CirGate*);
	void optDelGate(const CirGate*, const CirGateIn&);
	void delGate(const CirGate*);

	//simulation
	void iniFECList();
	void clearFECList();
	void firstSim();
	void sim(bool&);
	void correctFECfirst();
	void getSimLimit();
	void writeSimLog(vector<size_t>&);

	//fraig
	void createPfMod(SatSolver&);
	bool solvePair(unsigned, unsigned, SatSolver&);
	void getSATAssign(vector<size_t>&, SatSolver&);
	void getSATAssign(SatSolver&);
	void removeUnusedFEC();

	//update lists
	void updateUnused();
	void updateFloat();
	void updateDFS();
	void updateAll();
};

#endif // CIR_MGR_H
