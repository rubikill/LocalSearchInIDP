//
// Created by ck on 21.11.17.
//

#include "Localsearch.hpp"
#include "FirstImprovementSearch.hpp"


#include <inferences/querying/Query.hpp>
#include "inferences/modelexpansion/ModelExpansion.hpp"
#include "structure/MainStructureComponents.hpp"
#include "structure/StructureComponents.hpp"

#include <IncludeComponents.hpp>
#include "structure/Structure.hpp"
#include "theory/theory.hpp"
#include "theory/Query.hpp"
#include "vocabulary/vocabulary.hpp"
#include "NeighborhoodMove.hpp"
#include "TabuSearch.hpp"
#include <time.h>
#include "utils/ListUtils.hpp"

FirstImprovementSearch::FirstImprovementSearch(AbstractTheory* T, Query*  getObj, AbstractTheory* Tnext,
                       Query*  queryValidMoves, Query*  queryDeltaObj, Structure* str,Vocabulary* Vmove):
        LocalSearch(T, getObj, Tnext, queryValidMoves, queryDeltaObj,str, Vmove){
}

void FirstImprovementSearch::doSearch() {
    if(_verbosity >= 1)
        std::clog  << "hello, this is first improvement local search" <<"\n";
    auto inisol = getInitialSolution();              //return std::vector<Structure*>

//    std::clog<<"finish getting inital solution "<<std::endl;
//    inisol.get()->put(std::clog);

    _bestObjVal = _iniObjVal;
    _bestSol = inisol;                              //_initialSol;

    std::shared_ptr<Structure> currentSol = _initialSol;
    int currentobj = _iniObjVal;
    while(!timeout()){
        bool improved = false;

        NeighborhoodMove moves = getValidMoves(currentSol.get());
        TableIterator itermove = moves.begin();
        while(!itermove.isAtEnd()){
            //*iterator is ElementTuple&, or actually, std::vector<const DomainElement*>
            auto move =*itermove;
            auto solutionWithMove = decodeMove(currentSol.get(), move);
//            solutionWithMove->clean();
            auto deltaObj = getDeltaObj(solutionWithMove.get());
//            std::clog<<"process move "<<move<<" with deltaobj "<< deltaObj<<std::endl;

            if(deltaObj < 0) {              //if improves
                improved = true;

                auto newsol = getNeighborSolution(solutionWithMove.get());
//                std::clog<<"done getting neighbor solution"<<std::endl;
//                newsol.get()->put(std::clog);

                if(newsol == NULL)
                    break;

//                int newobj = currentobj + deltaObj;
//                currentobj = newobj;
                int newobj = LocalSearchUtility::getIntegerFromQuery(_queryGetObj, newsol.get());
                if(newobj < currentobj) {
                    if (_verbosity >= 1) {
                        std::clog << "\t move to new solution, obj = " << newobj << "\n";
                    }
                    currentSol = newsol;
                    currentobj = newobj;
                    if (newobj < _bestObjVal) {
                        _bestObjVal = newobj;
                        //TODO check if _bestSol is not iniSol then delete it
                        _bestSol = currentSol;
                    }
                    break;
                }
            }
            ++itermove;
        }
        if(!improved)
            break;
    }
}
