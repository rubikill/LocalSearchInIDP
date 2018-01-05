//
// Created by ck on 21.11.17.
//

#pragma once

#include <memory>

class LocalSearch;
class Structure;
class AbstractTheory;
class Vocabulary;
class Theory;
class Query;
class Sort;
class NeighborhoodMove;

class BestImprovementSearch: public LocalSearch{
public:
    BestImprovementSearch(AbstractTheory* T, Query*  getObj, AbstractTheory* Tnext,
    Query*  queryValidMoves, Query*  queryDeltaObj, Structure* str,Vocabulary* Vmove);
    void doSearch();
};


