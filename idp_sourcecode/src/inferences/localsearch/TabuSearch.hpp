//
// Created by ck on 16.11.17.
//

#pragma once

class LocalSearch;
class Structure;
class AbstractTheory;
class Vocabulary;
class Theory;
class Query;
class Sort;
class NeighborhoodMove;

class TabuSearch: public LocalSearch{
    int _tabuTenure;
    std::vector<std::vector<const DomainElement*>> _tabuList;
public:
    TabuSearch(AbstractTheory* T, Query*  getObj, AbstractTheory* Tnext,
               Query*  queryValidMoves, Query*  queryDeltaObj, Structure* str,Vocabulary* Vmove);
    void doSearch();
private:
    std::shared_ptr<Structure> getBestMove(const Structure* currentSol, NeighborhoodMove& listmoves);                //get best move, taken tabulist into account
    void updateTabuList(const std::vector<const DomainElement*>& move);
    int getMoveFromTabuList(const std::vector<const DomainElement*>& move);            //return index of move in tabulist
    void removeFromTabuList(int index);
    ~TabuSearch();
};