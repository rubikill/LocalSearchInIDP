//
// Created by coolkat on 6-11-17.
//

#include <fstream>
#include <inferences/querying/Query.hpp>
#include "Localsearch.hpp"
#include "inferences/modelexpansion/ModelExpansion.hpp"
#include "structure/MainStructureComponents.hpp"
#include "structure/StructureComponents.hpp"

#include <IncludeComponents.hpp>
#include "structure/Structure.hpp"
#include "theory/theory.hpp"
#include "theory/Query.hpp"
#include "vocabulary/vocabulary.hpp"
#include "NeighborhoodMove.hpp"

#include <time.h>
#include "utils/ListUtils.hpp"
#include "BestImprovementSearch.hpp"
#include "FirstImprovementSearch.hpp"
#include "TabuSearch.hpp"


LocalSearch::LocalSearch(AbstractTheory* T, Query*  queryGetObj, AbstractTheory* Tnext,
                         Query*  queryValidMoves, Query*  queryDeltaObj, Structure* str,Vocabulary* Vmove): _theo(T),
                                                                          _theoNext(Tnext), _queryGetObj(queryGetObj),
                                                                          _queryValidMoves(queryValidMoves),
                                                                          _queryDeltaObj(queryDeltaObj),
                                                                          _strucInput(str),
                                                                            _vocMove(Vmove){
    _voc = _theo->vocabulary();
    _vocNext = _theoNext->vocabulary();
    _timeinfor._timeout_second = getOption(IntType::TIMEOUT_LOCALSEARCH);
    _timeinfor._timeout_clock = _timeinfor._timeout_second * (CLOCKS_PER_SEC);
    _verbosity = getOption(IntType::VERBOSE_LOCALSEARCH);

    _timeinfor._timeStart = clock();
}

void LocalSearch::testmem(const Structure *str){
    int i = 0;
//    auto inisol = getInitialSolution();
    while(true){
        std::cout<<"initial solution "<<i<<std::endl;
//        _iniObjVal = LocalSearchUtility::getIntegerFromQuery(_queryGetObj, inisol);
        PredTable* result = Querying::doSolveQuery(_queryGetObj, str);
        delete result;
//        auto newstruc = _strucInput->clone();
//        auto models = ModelExpansion::doModelExpansion(_theo, newstruc, NULL)._models;        //_models is std::vector<Structure*>& v
//        delete newstruc;
//        for(int j = 0;j<models.size(); j++)
//            delete(models[j]);
        i++;
    }
}

std::vector<Structure*> LocalSearch::doLocalSearch(AbstractTheory *T, Structure *str, std::string* string){


    Namespace* ns = getGlobal()->getGlobalNamespace();
    Query *getObj = ns->query("objVal");
    AbstractTheory *Tnext = ns->theory("Tnext");
    Query *queryValidMoves = ns->query("getpossibleMoves");
    Query *queryDeltaObj = ns->query("queryGetDelObj");
    Vocabulary *Vmove = ns->vocabulary("Vmove");

    auto result = std::vector<Structure*>();
    LocalSearch* ls;

    switch(getGlobal()->getOptions()->localsearchType()){
        case LocalSearchType::BESTIMPROVE:
            ls = new BestImprovementSearch(T, getObj, Tnext, queryValidMoves,queryDeltaObj,str,Vmove);
//            ls->testmem(str);
            ls->doSearch();
            break;
        case LocalSearchType ::TABU:
            std::clog  << "hello, this is tabu search" <<"\n";
            ls = new TabuSearch(T, getObj, Tnext, queryValidMoves,queryDeltaObj,str,Vmove);
            ls->doSearch();
            break;
        case LocalSearchType::FIRSTIMPROVE:
        default:                //first improve
            ls = new FirstImprovementSearch(T, getObj, Tnext, queryValidMoves,queryDeltaObj,str,Vmove);
            ls->doSearch();
            break;
    }
    result.push_back(ls->_bestSol.get());

//    LocalSearchUtility::writeStructure(ls->_initialSol, "/home/ck/Desktop/idpexperiment/temp.txt");
    ls->_timeinfor._timeTotal =clock() - ls->_timeinfor._timeStart;
    ls->refineTimeInfor();

//    ls->_initialSol->put(std::clog);                //for TSP
    ls->printResult();

    return result;
}

int LocalSearch::getDeltaObj(Structure const *const structureWithMove){
    time_t start = clock();
    auto deltaObj = LocalSearchUtility::getIntegerFromQuery(_queryDeltaObj, structureWithMove);
    _timeinfor._timeGetDelObj += clock() - start;
    _timeinfor._countGetDelObj++;
    return deltaObj;
}
bool LocalSearch::timeout(){
    if((clock() - _timeinfor._timeStart) > _timeinfor._timeout_clock) {
        return true;
    }
    return false;
}
void LocalSearch::refineTimeInfor() {
    _timeinfor._timeTotal = _timeinfor._timeTotal/(CLOCKS_PER_SEC);
    _timeinfor._timeGetIniSol = _timeinfor._timeGetIniSol/(CLOCKS_PER_SEC);

    if(_timeinfor._countGetMove > 0) {
        _timeinfor._timeGetMove = _timeinfor._timeGetMove / _timeinfor._countGetMove;
        _timeinfor._timeGetMove = _timeinfor._timeGetMove/(CLOCKS_PER_SEC);
    }
    if(_timeinfor._countExpansionTnext > 0) {
        _timeinfor._timeExpansionTnext = _timeinfor._timeExpansionTnext / _timeinfor._countExpansionTnext;
        _timeinfor._timeExpansionTnext = _timeinfor._timeExpansionTnext/(CLOCKS_PER_SEC);
    }
    if(_timeinfor._countCreateNeighbor > 0) {
        _timeinfor._timeCreateNeighbor = _timeinfor._timeCreateNeighbor / _timeinfor._countCreateNeighbor;
        _timeinfor._timeCreateNeighbor = _timeinfor._timeCreateNeighbor/(CLOCKS_PER_SEC);
    }
    if(_timeinfor._countGetDelObj > 0) {
        _timeinfor._timeGetDelObj = _timeinfor._timeGetDelObj / _timeinfor._countGetDelObj;
        _timeinfor._timeGetDelObj = _timeinfor._timeGetDelObj/(CLOCKS_PER_SEC);
    }
}

void LocalSearch::printTimeInfor(){
    std::cout<<"time _timeTotal: "<<_timeinfor._timeTotal<<"\n";
    std::cout<<"time _timeGetIniSol: "<<_timeinfor._timeGetIniSol<<"\n";
    std::cout<<"time _timeGetMove: "<<_timeinfor._timeGetMove<<"\n";
    std::cout<<"time _timeExpansionTnext: "<<_timeinfor._timeExpansionTnext<<"\n";
    std::cout<<"time _timeCreateNeighbor: "<<_timeinfor._timeCreateNeighbor<<"\n";
    std::cout<<"time _timeGetDelObj: "<<_timeinfor._timeGetDelObj<<"\n";
}

void LocalSearch::printResult() {
    std::cout<<"best obj: "<<_bestObjVal<<std::endl;
    std::cout<<"ini sol: "<<_iniObjVal<<std::endl;
    printTimeInfor();
}
std::shared_ptr<Structure> LocalSearch::getInitialSolution(bool random) {
    time_t start = clock();

    auto models = ModelExpansion::doModelExpansion(_theo, _strucInput, NULL)._models;

    _timeinfor._timeGetIniSol = clock() - start;
    int index = 0;
    //TODO randommmm
//    if(random)
//        index = rand

    std::shared_ptr<Structure> inisol(models[index]);
    _initialSol = inisol;
    _iniObjVal = LocalSearchUtility::getIntegerFromQuery(_queryGetObj, _initialSol.get());
    LocalSearchUtility::cleanUpAfterModelExpand(models,index);

    if (_verbosity >= 1) {
        std::clog << "finish getting initial solution, obj = " << _iniObjVal << "\n";
    }

    return inisol;           //TODO: delete it when finish
}

std::shared_ptr<Structure> LocalSearch::getNeighborSolution(Structure* solutionWithMove){
    time_t start = clock();

    auto models = ModelExpansion::doModelExpansion(this->_theoNext, solutionWithMove, NULL)._models;        //_models is std::vector<Structure*>& v
    _timeinfor._timeExpansionTnext = clock() - start;
    Structure* solution_next = models[0];             //TODO check nb of models
    LocalSearchUtility::cleanUpAfterModelExpand(models,0);

    auto listfunc = this->_vocNext->getFuncs();        //std::map<std::string, Function*>&
    auto listpred = this->_vocNext->getPreds();         //std::map<std::string, Predicate*>&

    PFSymbol* dec_vars;             //    std::pair<PFSymbol*, PredInter*> dec_vars;
    PFSymbol* dec_next_vars;        //    std::pair<PFSymbol*, PredInter*> dec_next_path;

    auto prediter = listpred.begin();
    while(prediter != listpred.end()){
        auto tuple = *prediter;
        if(tuple.first.find("dec_") == 0 ) //&& != std::string::npos)// == "dec_Path/2")
        {
            dec_vars = tuple.second;
        }
        if(tuple.first.find("next_dec_") == 0) { //== "next_dec_Path/2")
            dec_next_vars = tuple.second;
        }
        ++prediter;
    }
    auto funcinter = listfunc.begin();              ////std::map<std::string, Function*>&
    while(funcinter != listfunc.end()){      // while(!funcinter._M_is_end()){
        auto tuple = *funcinter;
        if(tuple.first.find("dec_") == 0 ) { //&& != std::string::npos)// == "dec_Path/2")
            dec_vars = tuple.second;
        }
        if(tuple.first.find("next_dec_") == 0) { //== "next_dec_Path/2")
            dec_next_vars = tuple.second;
        }
        ++funcinter;
    }

    PredInter* inter_dec_next_vars = solution_next->inter(dec_next_vars)->clone();
    Structure* neighbor = this->_strucInput->clone();
    if(dec_vars->isPredicate()) {
        neighbor->changeInter(dynamic_cast<Predicate*>(dec_vars),inter_dec_next_vars);
    }
    else{       //if function
        FuncInter* finter = neighbor->inter(dynamic_cast<Function*> (dec_vars));
        finter->graphInter(inter_dec_next_vars);
    }
    neighbor->changeVocabulary(this->_voc);
    auto neighborexp = ModelExpansion::doModelExpansion(this->_theo, neighbor, NULL)._models;

    LocalSearchUtility::cleanUpAfterModelExpand(neighborexp,0);
    //TODO check empty() here

    delete(solution_next);
    delete(neighbor);
    _timeinfor._timeCreateNeighbor = clock() - start;
    _timeinfor._countCreateNeighbor++;
    _timeinfor._countExpansionTnext++;
    std::shared_ptr<Structure> result(neighborexp[0]);
    return result;
}



std::shared_ptr<Structure> LocalSearch::decodeMove(const Structure *s, const std::vector<const DomainElement *> &m)
{
    auto newstruc = std::shared_ptr<Structure>(s->clone());
    newstruc->changeVocabulary(_vocNext);

    //TODO NOTE the query output must be in the same order as vocabulary Vmove
    int i = 0;
    auto func = _vocMove->firstFunc();         //std::map<std::string, Function*>&
    while(func != _vocMove->lastFunc()) {
        if (func->second->builtin()) {
            ++func;
            continue;
        }
        auto funciter = newstruc->inter(func->second);
        PredInter* graph = funciter->graphInter();
        graph->makeTrueExactly({m[i]});
        funciter->materialize();
        ++func;
        i++;
    }
    return newstruc;
}
int LocalSearchUtility::getIntegerFromQuery(Query *q, Structure const *const s){
    PredTable* result = Querying::doSolveQuery(q, s);
    if(result->empty())
        std::clog  << "hello, error in getIntegerFromQuery here" <<"\n";
    TableIterator iterator = result->begin();
    auto detup = *iterator;         //ElementTuple&, or std::vector<const DomainElement*>
    auto data = detup.data();
    //TODO: check for error here, data includes 1 element only and it must be an integer
    if(data[0]->type()!= DET_INT)
        std::clog  << "error in getIntegerFromQuery - wrong type" <<std::endl;
    auto obj = data[0]->value()._int;
    delete result;
    return obj;
}

NeighborhoodMove LocalSearch::getValidMoves(Structure const * const s){
    time_t start = clock();
    PredTable* result = Querying::doSolveQuery(_queryValidMoves, s);
    NeighborhoodMove moves = NeighborhoodMove(result);

    if (_verbosity >= 1) {
        if(result->empty()) {
            std::clog << "no valid moves found" << "\n";
        }
    }
    _timeinfor._timeGetMove += clock()-start;
    _timeinfor._countGetMove++;
    return moves;
}

std::ostream& LocalSearch::put(std::ostream& s) const{
    s<<"best obj: "<<this->_bestObjVal<<" running time: "<<this->_timeinfor._timeTotal<<"\n";
    return s;
}

void LocalSearchUtility::writeStructure(const Structure *s, std::string filename) {
//    std::string b = "hallo";
//    std::string nametext = "Your name is " + name;

    std::ofstream myfile;
    myfile.open (filename);
    s->put(myfile);
    myfile.close();
}

void LocalSearchUtility::cleanUpAfterModelExpand(std::vector<Structure*>& models, int index){
    //delete all models except the one at index
    for(int i = 0;i<models.size();i++)
        if(i != index)
            delete(models[i]);
}