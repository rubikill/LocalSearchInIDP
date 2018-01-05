//
// Created by coolkat on 6-11-17.
//
#pragma once

#include <time.h>
#include <vector>
#include <iostream>
#include <memory>

class Structure;
class AbstractTheory;
class Vocabulary;
class Theory;
class Query;
class Sort;
class NeighborhoodMove;
class DomainElement;
class PredTable;

//struct lsData{
//    Theory* _theory;
//    Structure* _iniSol;
//    Structure* _bestSol;
//};

struct lsTimeInfor{
    int _timeout_second;
    long _timeout_clock;              //clock, be careful
    clock_t _timeStart;
    double _timeTotal = 0;
    double _timeGetIniSol =0;
    double _timeGetMove =0;
    double _timeExpansionTnext =0;
    double _timeCreateNeighbor =0;
    double _timeGetDelObj = 0;
    int _countGetMove = 0;
    int _countGetDelObj;
    int _countExpansionTnext=0;
    int _countCreateNeighbor=0;
};

class LocalSearch {
protected:
    AbstractTheory* _theo;
    AbstractTheory* _theoNext;
    Vocabulary* _voc;
    Vocabulary* _vocNext;
    Vocabulary* _vocMove;
    Query* _queryGetObj;
    Query* _queryValidMoves;
    Query* _queryDeltaObj;
    Structure* _strucInput;

    std::shared_ptr<Structure> _initialSol;
    std::shared_ptr<Structure> _bestSol;
    int _iniObjVal;
    int _bestObjVal;

    lsTimeInfor _timeinfor;
    int _verbosity;

public:
    static std::vector<Structure*> doLocalSearch(AbstractTheory *T, Structure *str, std::string* string);
protected:
    virtual void doSearch()=0;

    LocalSearch(AbstractTheory* T, Query*  queryGetObj, AbstractTheory* Tnext,
                Query*  queryValidMoves, Query*  queryDeltaObj, Structure* str,Vocabulary* Vmove);
    std::shared_ptr<Structure> getInitialSolution(bool random=false);
    std::shared_ptr<Structure> getNeighborSolution(Structure* s);
    int getDeltaObj(Structure const *const structureWithMove);
    std::shared_ptr<Structure> decodeMove(const Structure *s, const std::vector<const DomainElement *> &m);
    NeighborhoodMove  getValidMoves(Structure const * const s);
    std::ostream& put(std::ostream& s) const;
    bool timeout();
    void refineTimeInfor();
    void printTimeInfor();
    void printResult();
    void testmem(const Structure *str);
};

class LocalSearchUtility{
public:
    static int getIntegerFromQuery(Query *q, Structure const *const s);     //query must return a single integer
    static void writeStructure(const Structure* s, std::string filename);
    static void cleanUpAfterModelExpand(std::vector<Structure*>& models, int index);
};
