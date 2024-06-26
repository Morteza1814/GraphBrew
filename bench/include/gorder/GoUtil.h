#ifndef GOUTIL_H_
#define GOUTIL_H_

#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <climits>
#include <ctime>
#include <cmath>
#include <vector>
#include <string>
#include <cstring>

namespace GorderUtil
{

using namespace std;

unsigned long long MyRand64(){
	unsigned long long ret, tmp;
	ret=rand();
	ret=ret<<33;
	tmp=rand();
	ret=ret|(tmp<<2);
	tmp=rand();
	ret=ret|(tmp>>29);

	return ret;
}

string extractFilename(const char* filename){
	string name(filename);
	int pos=name.find_last_of('.');

	return name.substr(0, pos);
}

void quit(){
	// int ret = system("pause");
	exit(0);
}

template<class T>
void VectorPreprocessing(vector<T>& v, T u){
	if(v.size()<2)
		return ;

	int p1, p2;
	
	sort(v.begin(), v.end());
	p1=0;
	p2=1;
	while(v[0]==u){
		v.erase(v.begin());
	}

	while(p2<v.size()){
		if(v[p2]==u || v[p1]==v[p2]){
			v.erase(v.begin()+p2);
			continue;
		}
		p1++;
		p2++;
	}
	
}


inline int IntersectionSize(const int* v1, const int* v2, int s1, int s2, int u){
	int i=upper_bound(v1, v1+s1, u)-v1;
	int j=lower_bound(v2, v2+s2, v1[i])-v2;
	int count=0;

	while(i<s1&&j<s2){
		if(v1[i]<v2[j]){
			i++;
		} else if(v1[i]>v2[j]){
			j++;
		} else {
			count++;
			i++;
			j++;
		}
	}

	return count;
}


template<class T>
bool VectorEq(const vector<T>& v1, const vector<T>& v2){
	if(v1.size()!=v2.size())
		return false;

	for(int i=0; i<v1.size(); i++){
		if(v1[i]!=v2[i])
			return false;
	}

	return true;
}


template<class T>
bool IsIntersect(const vector<T>& v1, const vector<T>& v2){
    int i=0, j=0;

    while(i<v1.size()&&j<v2.size()){
        if(v1[i]==v2[j]){
			return true;
        }
        if(v1[i]<v2[j]){
			i++;
        }
        if(v1[i]>v2[j]){
			j++;
        }
    }

    return false;
}

template<class T1, class T2>
class PairCompare{
	bool flag;

public:
	PairCompare(bool in){
		flag=in;
	}
	bool operator()(const pair<T1, T2>& p1, const pair<T1, T2>& p2){
		if(flag)
			return p1.second < p2.second;
		else
			return p1.second > p2.second;
	}

};

template<class T1, class T2>
class PairComparePointer{
public:
	bool operator()(const pair<T1, T2>& p1, const pair<T1, T2>& p2){
		return *(p1.second) > *(p2.second);
	}
};

template<class T>
inline T MyMin(const T& left, const T& right){
	if(left<right)
		return left;
	else
		return right;
}


template<class T1, class T2>
class ReRank{
	const vector<T2>& value;
public:
	ReRank(vector<T2>& v): value(v){
	}

	bool operator()(const T1& left, const T1& right){
		return value[left] < value[right];
	}

};



}

#endif

