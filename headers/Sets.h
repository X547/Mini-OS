#ifndef _SETS_H_
#define _SETS_H_

template<typename Base>
class Set {
private:
	Base fBits;

public:
	Set(): fVal(0) {}
	Set(int bit): fBits((Base)(1) << bit) {}
	Set(int begin, int end): fBits((((Base)(1) << (end + 1)) - 1) & ~(((Base)(1) << beg) - 1)) {}

	bool Contains(int val) const
	{
		return (Set(val).fBits & fBits) != 0;
	}

	Set operator |(Set other) const
	{
		Set res;
		res.fBits = fBits | other.fBits;
		return res;
	}

	Set operator &(Set other) const
	{
		Set res;
		res.fBits = fBits & other.fBits;
		return res;
	}

};

#endif	// _SETS_H_
