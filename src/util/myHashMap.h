/****************************************************************************
  FileName     [ myHashMap.h ]
  PackageName  [ util ]
  Synopsis     [ Define HashMap and Cache ADT ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2009-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef MY_HASH_MAP_H
#define MY_HASH_MAP_H

#include <vector>

using namespace std;

// TODO: Implement your own HashMap and Cache classes.

//-----------------------
// Define HashMap classes
//-----------------------
// To use HashMap ADT, you should define your own HashKey class.
// It should at least overload the "()" and "==" operators.
//
// class HashKey
// {
// public:
//    HashKey() {}
// 
//    size_t operator() () const { return 0; }
// 
//    bool operator == (const HashKey& k) const { return true; }
// 
// private:
// };
//

class GateHashKey
{
public:
	GateHashKey(unsigned i, unsigned j): in0(i), in1(j) {}
	size_t operator()() const { return (size_t)in0 * (size_t)in0 * (size_t)in0 + (size_t)in1 * (size_t)in1 * (size_t)in1; }
	bool operator ==(const GateHashKey& k) const { return((in0 == k.in0 && in1 == k.in1) || (in0 == k.in1 && in1 == k.in0));}

private:
	unsigned in0, in1;
};

class SimHashKey
{
public:
	SimHashKey(size_t i): keyValue(i) {}
	size_t operator()() const { return keyValue; }
	bool operator ==(const SimHashKey& k) const { return(keyValue == k.keyValue); }
	SimHashKey operator ~() const { return SimHashKey(~keyValue); }
private:
	size_t keyValue;
};

class DFSHashKey
{
public:
	DFSHashKey(unsigned k, unsigned v = 0): key(k), value(v) {}
	DFSHashKey(const DFSHashKey & k): key(k.key), value(k.value) {}
	size_t operator()() const { return key; }
	bool operator ==(const DFSHashKey& k) const { return(key == k.key); }

	void setvalue(unsigned v) { value = v; }
	unsigned getvalue() { return value; }
	
private:
	unsigned key;
	unsigned value;

};

template <class HashKey, class HashData>
class HashMap
{
typedef pair<HashKey, HashData> HashNode;

public:
	HashMap() : _numBuckets(0), _buckets(0) {}
	HashMap(size_t b) : _numBuckets(0), _buckets(0) { init(b); }
	~HashMap() { reset(); }

	// [Optional] TODO: implement the HashMap<HashKey, HashData>::iterator
	// o An iterator should be able to go through all the valid HashNodes
	//   in the HashMap
	// o Functions to be implemented:
	//   - constructor(s), destructor
	//   - operator '*': return the HashNode
	//   - ++/--iterator, iterator++/--
	//   - operators '=', '==', !="
	//
	// (_bId, _bnId) range from (0, 0) to (_numBuckets, 0)
	//
	class iterator
	{
		friend class HashMap<HashKey, HashData>;

	public:
		iterator(HashMap<HashKey, HashData>* h = 0, size_t b = 0, size_t bn = 0)
		: _hash(h), _bId(b), _bnId(bn) {}
		iterator(const iterator& i)
		: _hash(i._hash), _bId(i._bId), _bnId(i._bnId) {}
		~iterator() {} // Should NOT delete HashData

		const HashNode& operator * () const { return (*_hash)[_bId][_bnId]; }
		HashNode& operator * () { return (*_hash)[_bId][_bnId]; }
		iterator& operator ++ () {
			if (_hash == 0) return (*this);
			if (_bId >= _hash->_numBuckets) return (*this);
			if (++_bnId >= (*_hash)[_bId].size()) {
				while ((++_bId < _hash->_numBuckets) && (*_hash)[_bId].empty());
				_bnId = 0;
			}
			return (*this);
		}
		iterator& operator -- () {
			if (_hash == 0) return (*this);
			if (_bnId == 0) {
				if (_bId == 0) return (*this);
				while ((*_hash)[--_bId].empty())
					if (_bId == 0) return (*this);
				_bnId = (*_hash)[_bId].size() - 1;
			}
			else
				--_bnId;
			return (*this);
		}
		iterator operator ++ (int) { iterator li=(*this); ++(*this); return li; }
		iterator operator -- (int) { iterator li=(*this); --(*this); return li; }

		iterator& operator = (const iterator& i) {
			_hash = i._hash; _bId = i._bId; _bnId = i._bnId; return (*this); }

		bool operator != (const iterator& i) const { return !(*this == i); }
		bool operator == (const iterator& i) const {
			return (_hash == i._hash && _bId == i._bId && _bnId == i._bnId); }

	private:
		HashMap<HashKey, HashData>*   _hash;
		size_t                        _bId;
		size_t                        _bnId;
	};

	void init(size_t b) {
		reset(); _numBuckets = b; _buckets = new vector<HashNode>[b]; }
	void reset() {
		_numBuckets = 0;
		if (_buckets) { delete [] _buckets; _buckets = 0; }
	}
	size_t numBuckets() const { return _numBuckets; }

	vector<HashNode>& operator [] (size_t i) { return _buckets[i]; }
	const vector<HashNode>& operator [](size_t i) const { return _buckets[i]; }

	// TODO: implement these functions
	//
	// Point to the first valid data
	iterator begin() const {
		if (_buckets == 0) return end();
/*
		size_t i = 0;
		while (_buckets[i].empty()) ++i;
		if (i == _numBuckets) return end();
		return iterator(const_cast<HashMap<HashKey, HashData>*>(this), i, 0);
*/
		for (size_t i = 0; i < _numBuckets; ++i)
			if (!_buckets[i].empty())
				return iterator(const_cast<HashMap<HashKey, HashData>*>(this), i, 0);
		return end();
	}
	// Pass the end
	iterator end() const {
		return iterator(const_cast<HashMap<HashKey, HashData>*>(this),
				 _numBuckets, 0);
	}
	// return true if no valid data
	bool empty() const {
		for (size_t i = 0; i < _numBuckets; ++i)
			if (_buckets[i].size() != 0) return false;
		return true;
	}
	// number of valid data
	size_t size() const {
		size_t s = 0;
		for (size_t i = 0; i < _numBuckets; ++i) s += _buckets[i].size();
		return s;
	}

	// check if k is in the hash...
	// if yes, update n and return true;
	// else return false;
	bool check(const HashKey& k, HashData& n) const {
		size_t b = bucketNum(k);
		for (size_t i = 0, bn = _buckets[b].size(); i < bn; ++i)
			if (_buckets[b][i].first == k) {
				n = _buckets[b][i].second;
				return true;
			}
		return false;
	}

	// return true if inserted successfully (i.e. k is not in the hash)
	// return false is k is already in the hash ==> will not insert
	bool insert(const HashKey& k, const HashData& d) {
		size_t b = bucketNum(k);
		for (size_t i = 0, bn = _buckets[b].size(); i < bn; ++i)
			if (_buckets[b][i].first == k)
				return false;
		_buckets[b].push_back(HashNode(k, d));
		return true;
	}

	// return true if inserted successfully (i.e. k is not in the hash)
	// return false is k is already in the hash ==> still do the insertion
	bool replaceInsert(const HashKey& k, const HashData& d) {
		size_t b = bucketNum(k);
		for (size_t i = 0, bn = _buckets[b].size(); i < bn; ++i)
			if (_buckets[b][i].first == k) {
				_buckets[b][i].second = d;
				return false;
			}
		_buckets[b].push_back(HashNode(k, d));
		return true;
	}

	// Need to be sure that k is not in the hash
	void forceInsert(const HashKey& k, const HashData& d) {
		_buckets[bucketNum(k)].push_back(HashNode(k, d)); }

private:
	// Do not add any extra data member
	size_t                   _numBuckets;
	vector<HashNode>*        _buckets;

	size_t bucketNum(const HashKey& k) const {
		return (k() % _numBuckets); }

};


//---------------------
// Define Cache classes
//---------------------
// To use Cache ADT, you should define your own HashKey class.
// It should at least overload the "()" and "==" operators.
//
// class CacheKey
// {
// public:
//    CacheKey() {}
//    
//    size_t operator() () const { return 0; }
//   
//    bool operator == (const CacheKey&) const { return true; }
//       
// private:
// }; 
// 
template <class CacheKey, class CacheData>
class Cache
{
typedef pair<CacheKey, CacheData> CacheNode;

public:
	Cache() : _size(0), _cache(0) {}
	Cache(size_t s) : _size(0), _cache(0) { init(s); }
	~Cache() { reset(); }

	// NO NEED to implement Cache::iterator class

	// TODO: implement these functions
	//
	// Initialize _cache with size s
	void init(size_t s) { reset(); _size = s; _cache = new CacheNode[s]; }
	void reset() {  _size = 0; if (_cache) { delete [] _cache; _cache = 0; } }

	size_t size() const { return _size; }

	CacheNode& operator [] (size_t i) { return _cache[i]; }
	const CacheNode& operator [](size_t i) const { return _cache[i]; }

	// return false if cache miss
	bool read(const CacheKey& k, CacheData& d) const {
		size_t i = k() % _size;
		if (k == _cache[i].first) {
			d = _cache[i].second;
			return true;
		}
		return false;
	}
	// If k is already in the Cache, overwrite the CacheData
	void write(const CacheKey& k, const CacheData& d) {
		size_t i = k() % _size;
		_cache[i].first = k;
		_cache[i].second = d;
	}

private:
	// Do not add any extra data member
	size_t         _size;
	CacheNode*     _cache;
};

template <class Data>
class HashSet
{
public:
	HashSet(size_t b = 0) : _numBuckets(0), _buckets(0) { if (b != 0) init(b); }
	~HashSet() { reset(); }

	class iterator
	{
		friend class HashSet<Data>;

	public:
		iterator(vector<Data>* n, size_t i, vector<Data>* e): _node(n), _it(i), _end(e) {}
		iterator(const iterator& i): _node(i._node), _it(i._it), _end(i._end) {}
		~iterator() {} 

		const Data& operator * () const { return (*_node)[_it]; }
		Data& operator * () { return (*_node)[_it]; }
		iterator& operator ++ () {
			do {
				if(_it + 1 < (*_node).size()) ++_it;
				else { ++_node; _it = 0; }
			} while((*_node).empty() && _node != _end); 
			return (*this);
		}
		iterator operator ++ (int) { iterator temp(*this); ++(*this); return temp; }
		iterator& operator -- () {
			do {
				if(_it > 0) --_it;
				else { --_node; 
					if(!(*_node).empty()) _it = (*_node).size() - 1;
					else _it = 0;
				}
			} while((*_node).empty()); 
			return (*this);
		}
		iterator operator -- (int) { iterator temp(*this); --(*this); return temp; }
		iterator& operator = (const iterator& i) { _node = i._node; _it = i._it;return (*this); }
		bool operator != (const iterator& i) const { return (_node != i._node || _it != i._it); }
		bool operator == (const iterator& i) const { return (_node == i._node && _it == i._it); }

	private:
		vector<Data>* _node;
		size_t _it;
		vector<Data>* _end;
	};

	void init(size_t b) { _numBuckets = b; _buckets = new vector<Data>[b]; }
	void reset() {
		_numBuckets = 0;
		if (_buckets) { delete [] _buckets; _buckets = 0; }
	}
	void clear() {
		for (size_t i = 0; i < _numBuckets; ++i) _buckets[i].clear();
	}
	size_t numBuckets() const { return _numBuckets; }

	vector<Data>& operator [] (size_t i) { return _buckets[i]; }
	const vector<Data>& operator [](size_t i) const { return _buckets[i]; }

	// TODO: implement these functions
	//
	// Point to the first valid data
	iterator begin() const {
		for(size_t i = 0; i < _numBuckets; ++i) {
			if(!_buckets[i].empty()) return iterator(_buckets + i, 0, _buckets + _numBuckets);
		} 
		return end();
	}
	// Pass the end
	iterator end() const {
		return iterator(_buckets + _numBuckets, 0, _buckets + _numBuckets);
	}
	// return true if no valid data
	bool empty() const {
		for(size_t i = 0; i < _numBuckets; ++i) {
			if(!_buckets[i].empty()) return false;
		} 
		return true;
	}
	// number of valid data
	size_t size() const {
		size_t count = 0;
		for(size_t i = 0; i < _numBuckets; ++i) {
			count += _buckets[i].size();
		} 
		return count;
	}

	// check if d is in the hash...
	// if yes, return true;
	// else return false;
	bool check(const Data& d) const {
		for(size_t i = 0; i < _buckets[bucketNum(d)].size(); ++i) {
			if(d == _buckets[bucketNum(d)][i]) return true;
		}
		return false;
	}

	// query if d is in the hash...
	// if yes, replace d with the data in the hash and return true;
	// else return false;
	bool query(Data& d) const {
		for(size_t i = 0; i < _buckets[bucketNum(d)].size(); ++i) {
			if(d == _buckets[bucketNum(d)][i]) {
				d = _buckets[bucketNum(d)][i];
				return true;
			}
		}
		return false;
	}

	// update the entry in hash that is equal to d
	// if found, update that entry with d and return true;
	// else insert d into hash as a new entry and return false;
	bool update(const Data& d) {
		for(size_t i = 0; i < _buckets[bucketNum(d)].size(); ++i) {
			if(d == _buckets[bucketNum(d)][i]) {
				_buckets[bucketNum(d)][i] = d;
				return true;
			}
		}
		_buckets[bucketNum(d)].push_back(d);
		return false;
	}

	// return true if inserted successfully (i.e. d is not in the hash)
	// return false is d is already in the hash ==> will not insert
	bool insert(const Data& d) {
		if(check(d)) return false;
		_buckets[bucketNum(d)].push_back(d);
		return true;
	}

	// return true if removed successfully (i.e. d is in the hash)
	// return fasle otherwise (i.e. nothing is removed)
	bool remove(const Data& d) {
		for(size_t i = 0; i < _buckets[bucketNum(d)].size(); ++i) {
			if(d == _buckets[bucketNum(d)][i]) {
				_buckets[bucketNum(d)].erase(_buckets[bucketNum(d)].begin() + i);
				return true;
			}
		}
		return false;
	}

	private:
	// Do not add any extra data member
	size_t            _numBuckets;
	vector<Data>*     _buckets;

	size_t bucketNum(const Data& d) const {
		return (d % _numBuckets); }
};

#endif // MY_HASH_H
