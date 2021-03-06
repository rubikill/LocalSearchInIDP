/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#include "CalculateDefinitions.hpp"
#include "inferences/SolverConnection.hpp"
#include "fobdds/FoBddManager.hpp"

#include "theory/TheoryUtils.hpp"
#include "vocabulary/vocabulary.hpp"
#include "utils/ListUtils.hpp"
#include "structure/StructureComponents.hpp"

#include "groundtheories/SolverTheory.hpp"

#include "inferences/grounding/grounders/Grounder.hpp"
#include "inferences/grounding/LazyGroundingManager.hpp"
#include "inferences/grounding/GrounderFactory.hpp"
#include "inferences/grounding/GroundTranslator.hpp"
#include "inferences/propagation/PropagatorFactory.hpp"

#ifdef WITHXSB
#include "inferences/querying/xsb/XSBInterface.hpp"
#endif

#include "options.hpp"
#include <iostream>

using namespace std;

CalculateDefinitions::CalculateDefinitions(Theory* t, Structure* s, Vocabulary* vocabulary, bool satdelay) :
	_theory(t), _structure(s), _satdelay(satdelay), _tooExpensive(false) {
	if (_theory == NULL || _structure == NULL) {
		throw IdpException("Unexpected NULL-pointer.");
	}
	_symbolsToQuery = getSet(vocabulary->getNonBuiltinNonOverloadedNonTypeSymbols());
#ifdef DEBUG
	for (auto symbol : _symbolsToQuery) {
		if (not _structure->vocabulary()->contains(symbol)) {
			stringstream ss;
			ss << "Asked to evaluate symbol " << symbol->name() << ", but this is impossible because it is not in the vocabulary of the given structure";
			throw IdpException(ss.str());
		}
	}
#endif DEBUG
	if (getOption(VERBOSE_DEFINITIONS) > 1) {
		clog << "Evaluating the following symbols: ";
		printList(clog,_symbolsToQuery,",",true);
		clog << endl;
	}
	if (getOption(VERBOSE_DEFINITIONS) > 0) {
		std::set<PFSymbol*> symbolsUnableToEvaluate;
		auto inputStarSymbols = determineInputStarSymbols(t);
		for (auto symbol : _symbolsToQuery) {
			if (not contains(inputStarSymbols,symbol)) {
				symbolsUnableToEvaluate.insert(symbol);
			}
		}
		if (not symbolsUnableToEvaluate.empty()) {
			clog << "CalculateDefinitions is NOT able to calculate the following symbols, even though it was (implicitly) asked: ";
			printList(clog,_symbolsToQuery,",",true);
			clog << endl;
		}
	}
}


DefinitionCalculationResult CalculateDefinitions::doCalculateDefinitions(
		Theory* theory, Structure* structure, bool satdelay) {
	return doCalculateDefinitions(theory,structure,theory->vocabulary(),satdelay);
}

DefinitionCalculationResult CalculateDefinitions::doCalculateDefinitions(
		Theory* theory, Structure* structure, Vocabulary* vocabulary, bool satdelay) {
	CalculateDefinitions c(theory, structure, vocabulary, satdelay);
	return c.calculateKnownDefinitions();
}

DefinitionCalculationResult CalculateDefinitions::doCalculateDefinition(
	const Definition* definition, Structure* structure, bool satdelay) {
	auto vocabulary = new Vocabulary("wrapper_vocabulary");
	for (auto symbol : definition->defsymbols()) {
		vocabulary->add(symbol);
	}
	auto ret = doCalculateDefinition(definition,structure,vocabulary,satdelay);
	delete(vocabulary);
	return ret;
}

DefinitionCalculationResult CalculateDefinitions::doCalculateDefinition(
	const Definition* definition, Structure* structure, Vocabulary* vocabulary, bool satdelay) {
	auto theory = new Theory("wrapper_theory", vocabulary, ParseInfo());
	auto newdef = definition->clone();
	theory->add(newdef);
	auto ret = doCalculateDefinitions(theory, structure, vocabulary, satdelay);
	theory->recursiveDelete();
	return ret;
}


DefinitionCalculationResult CalculateDefinitions::calculateDefinition(const Definition* definition) {
	if (getOption(IntType::VERBOSE_DEFINITIONS) >= 2) {
		clog << "Calculating definition: " << toString(definition) << "\n";
		if (getOption(IntType::VERBOSE_DEFINITIONS) >= 4) {
			clog << "Using structure " << toString(_structure) << "\n";
		}
	}
	DefinitionCalculationResult result(_structure);
	result._hasModel = true;
#ifdef WITHXSB
	auto withxsb = CalculateDefinitions::determineXSBUsage(definition);
	if (withxsb) {
		if(_satdelay or getOption(SATISFIABILITYDELAY)) { // TODO implement checking threshold by size estimation (see issue #850)
			Warning::warning("Lazy threshold is not checked for definitions evaluated with XSB");
		}
		if (getOption(IntType::VERBOSE_DEFINITIONS) >= 2) {
			clog << "Calculating the above definition using XSB\n";
		}
		auto xsb_interface = XSBInterface::instance();
		xsb_interface->load(definition,_structure);
		std::vector<PFSymbol*> symbols(definition->defsymbols().begin(), definition->defsymbols().end()); // vector so we can use remove_if
		std::function<bool(PFSymbol*)> ignoreSymbol = [&](PFSymbol* s){return _symbolsToQuery.find(s) == _symbolsToQuery.end();};
		symbols.erase(std::remove_if(symbols.begin(),symbols.end(),ignoreSymbol),symbols.end());
		std::function<DefinitionCalculationResult(bool)> onFail = [&](bool doReset){
			if (doReset) xsb_interface->reset();
			result._hasModel=false;
			return result;
		};
		if (definitionDoesNotResultInTwovaluedModel(definition)) {
			return onFail(false);
		}
		for (auto symbol : symbols) {
			auto sorted = xsb_interface->queryDefinition(symbol);
			auto internpredtable1 = new EnumeratedInternalPredTable(sorted);
			auto predtable1 = new PredTable(internpredtable1, _structure->universe(symbol));
			if(not isConsistentWith(predtable1, _structure->inter(symbol))){
				delete(predtable1);
				return onFail(true);
			}
			_structure->inter(symbol)->ctpt(predtable1);
			delete(predtable1);
			_structure->clean();
			if(isa<Function>(*symbol)) {
				auto fun = dynamic_cast<Function*>(symbol);
				if(not _structure->inter(fun)->approxTwoValued()){ // E.g. for functions
					return onFail(true);
				}
			}
			if(not _structure->inter(symbol)->isConsistent()){
				return onFail(true);
			}
		}

		xsb_interface->reset();
		if (not _structure->isConsistent()) {
			return onFail(false);
		} else {
			result._hasModel=true;
		}
		return result;
	}
#endif
	// Default: Evaluation using ground-and-solve
	auto data = SolverConnection::createsolver(1);
	auto theory = new Theory("", _structure->vocabulary(), ParseInfo());
	theory->add(definition->clone());
	bool LUP = getOption(BoolType::LIFTEDUNITPROPAGATION);
	bool propagate = LUP || getOption(BoolType::GROUNDWITHBOUNDS);
	auto symstructure = generateBounds(theory, _structure, propagate, LUP);
	auto grounder = GrounderFactory::create(GroundInfo(theory, { _structure, symstructure }, NULL, true), data);

	auto size = toDouble(grounder->getMaxGroundSize());
	size = size < 1 ? 1 : size;
	if ((_satdelay or getOption(SATISFIABILITYDELAY)) and log(size) / log(2) > 2 * getOption(LAZYSIZETHRESHOLD)) {
		      _tooExpensive = true;
		delete (data);
		delete (grounder);
		result._hasModel=true;
		theory->recursiveDelete();
		return result;
	}

	bool unsat = grounder->toplevelRun();

	//It's possible that unsat is found (for example when we have a conflict with function constraints)
	if (unsat) {
		// Cleanup
		delete (data);
		delete (grounder);
		result._hasModel=false;
		theory->recursiveDelete();
		return result;
	}

	Assert(not unsat);
	AbstractGroundTheory* grounding = dynamic_cast<SolverTheory*>(grounder->getGrounding());

	// Run solver
	data->finishParsing();
	auto mx = SolverConnection::initsolution(data, 1);
	mx->execute();
	if (getGlobal()->terminateRequested()) {
		throw IdpException("Solver was terminated");
	}

	// Collect solutions
	auto abstractsolutions = mx->getSolutions();
	if (not abstractsolutions.empty()) {
		Assert(abstractsolutions.size() == 1);
		auto model = *(abstractsolutions.cbegin());
		SolverConnection::addLiterals(*model, grounding->translator(), _structure);
		SolverConnection::addTerms(*model, grounding->translator(), _structure);
		      _structure->clean();
	}
	for (auto symbol : definition->defsymbols()) {
		if (isa<Function>(*symbol)) {
			auto fun = dynamic_cast<Function*>(symbol);
			if (not _structure->inter(fun)->approxTwoValued()) { // Check for functions that are defined badly
				result._hasModel = false;
			}
		}
	}

	// Cleanup
	grounding->recursiveDelete();
	theory->recursiveDelete();
	delete (data);
	delete (mx);
	delete (grounder);

	result._hasModel &= (not abstractsolutions.empty() && _structure->isConsistent());
	return result;
}

DefinitionCalculationResult CalculateDefinitions::calculateKnownDefinitions() {
	if(_theory->definitions().empty()) {
		DefinitionCalculationResult result(_structure);
		result._hasModel = true;
		return result;
	}
#ifdef WITHXSB
	if (getOption(XSB)) {
		DefinitionUtils::joinDefinitionsForXSB(_theory, _structure);
	}
#endif
	_theory = FormulaUtils::improveTheoryForInference(_theory, _structure, false, false, false);
	if (not _symbolsToQuery.empty()) {
		updateSymbolsToQuery(_symbolsToQuery, _theory->definitions()); // TODO: Adds too much symbols! Instead, relax reqs below!
	}
	if (getOption(IntType::VERBOSE_DEFINITIONS) >= 1) {
		clog << "Calculating known definitions";
		if (getOption(IntType::VERBOSE_DEFINITIONS) >= 3) {
			clog << " of (at most) the following symbols:\n\t";
			printList(clog,_symbolsToQuery,",\n\t",true);
		}
		clog << "..." << endl;
	}
	auto opens = DefinitionUtils::opens(_theory->definitions());// Collect the open symbols of all definitions
	if (getOption(BoolType::STABLESEMANTICS)) {
		CalculateDefinitions::removeNonTotalDefnitions(opens);
	}
	DefinitionCalculationResult result(_structure);
	result._hasModel = true;

	// Calculate the interpretation of the defined atoms from definitions that do not have three-valued open symbols
	bool fixpoint = false;
	while (not fixpoint and result._hasModel) {
		fixpoint = true;
		for (auto it = opens.begin(); it != opens.end();) {
			auto currentdefinition = it++; // REASON: set erasure does only invalidate iterators pointing to the erased elements

			// Remove opens that have a two-valued interpretation
			auto toRemove = DefinitionUtils::approxTwoValuedOpens(currentdefinition->first, _structure);
			for (auto symbol : toRemove) {
				if (currentdefinition->second.find(symbol) != currentdefinition->second.end()) {
					currentdefinition->second.erase(symbol);
				}
			}

			// If no opens are left, calculate the interpretation of the defined atoms
			if (currentdefinition->second.empty()) {
				auto definition = currentdefinition->first;
				bool tooexpensive = false;
				auto defCalcResult = calculateDefinition(definition);
				if (tooexpensive) {
					continue;
				}
				result._calculated_model = defCalcResult._calculated_model; // Update current structure

				if (not defCalcResult._hasModel) { // If the definition did not have a model, quit execution (don't set fixpoint to false)
					if (getOption(IntType::VERBOSE_DEFINITIONS) >= 1) {
						clog << "The given structure cannot be extended to a model of the definition\n" << toString(definition) << "\n";
					}
					result._hasModel = false;
				} else { // If it did have a model, update result and continue
					fixpoint = false;
					opens.erase(currentdefinition);
					               _theory->remove(definition);
					definition->recursiveDelete();
				}
			}
		}
	}
	if (not result._hasModel or not result._calculated_model->isConsistent()) {
		// When returning a result that has no model, the other arguments are as follows:
		// _calculated_model:  the structure resulting from the last unsuccessful definition calculation
		// _calculated_definitions: all definitions that have been successfully calculated
		result._hasModel = false;
		return result;
	}
	if (getOption(IntType::VERBOSE_DEFINITIONS) >= 1) {
		clog << "Done calculating known definitions\n";
		if (getOption(IntType::VERBOSE_DEFINITIONS) >= 4) {
			clog << "Resulting structure:\n" << toString(_structure) << "\n";
		}
	}
	result._hasModel = true;
	return result;
}


void CalculateDefinitions::removeNonTotalDefnitions(std::map<Definition*,
		std::set<PFSymbol*> >& opens) {
	bool foundone = false;
	auto def = opens.begin();
	while (def != opens.end()) {
		auto hasrecursion = DefinitionUtils::approxHasRecursionOverNegation((*def).first);
		//TODO in the future: put a smarter check here

		auto currentdefinition = def++;
		// REASON: set erasure does only invalidate iterators pointing to the erased elements

		if (hasrecursion) {
			foundone = true;
			opens.erase(currentdefinition);
		}
	}
	if (foundone) {
		Warning::warning("Ignoring definitions for which we cannot detect totality because option stablesemantics is true.");
	}
}

void CalculateDefinitions::updateSymbolsToQuery(std::set<PFSymbol*>& symbols, std::vector<Definition*> defs) const {
	std::set<PFSymbol*> opens;
	bool fixpoint = false;
	while (not fixpoint) {
		fixpoint = true;
		for (auto def : defs) {
			for (auto symbol : symbols) {
				if (def->defsymbols().find(symbol) != def->defsymbols().end()) {
					auto dependenciesOfSymbol = DefinitionUtils::getDirectDependencies(def,symbol);
					for (auto dependency : dependenciesOfSymbol) {
						if (dependency == symbol) { continue; }
						for (auto def2 : defs) {
							if (def2 == def) {
								continue;
							}
							if (def2->defsymbols().find(dependency) != def2->defsymbols().end()) {
								// There is an "external" definition that defines the required
								// dependency, so it must also be queried (and inserted into IDP)
								if (symbols.insert(dependency).second) { // .second return value indicates whether it was a "new" element in the set
									fixpoint = false;
								}
							}
						}
					}
				}
			}
		}
	}
}

std::set<PFSymbol *> CalculateDefinitions::determineInputStarSymbols(Theory* t) const {
	set<PFSymbol*> inputstar;
	bool fixpoint = false;
	while(not fixpoint) {
		fixpoint = true;
		for (auto def : t->definitions()) {
			auto oldsize = inputstar.size();
			addNewInputStar(def,inputstar);
			if (oldsize <  inputstar.size()) {
				fixpoint = false;
			}
		}
	}
	return inputstar;
}


void CalculateDefinitions::addNewInputStar(const Definition* d, std::set<PFSymbol*>& inputstar) const {
	addAll(inputstar, DefinitionUtils::approxTwoValuedOpens(d,_structure));
	set<PFSymbol*> potentialNewSymbols = DefinitionUtils::defined(d);
	bool fixpoint = false;
	while (not fixpoint) {
		fixpoint = true;
		for (auto it = potentialNewSymbols.begin(); it != potentialNewSymbols.end();) {
			auto defsymbol = *(it++); // potential set erasure
			auto deps = DefinitionUtils::getDirectDependencies(d, defsymbol);
			if (isSubset(deps,inputstar)) { // i.e., all dependencies are inputstar
				inputstar.insert(defsymbol);
				fixpoint = false;
				potentialNewSymbols.erase(defsymbol);
			}
		}
	}
}

bool CalculateDefinitions::definitionDoesNotResultInTwovaluedModel(const Definition* definition) const {
	auto possRecNegSymbols = DefinitionUtils::approxRecurionsOverNegationSymbols(definition);
	auto xsb_interface = XSBInterface::instance();
	for (auto symbol : possRecNegSymbols) {
		if(xsb_interface->hasUnknowns(symbol)) {
			xsb_interface->reset();
			return true;
		}
	}
	return false;
}


#ifdef WITHXSB
bool CalculateDefinitions::determineXSBUsage(const Definition* definition) {
	if (not (getOption(XSB))) {
		return false;
	}
	if (DefinitionUtils::approxContainsRecDefAggTerms(definition)) {
		Warning::warning("Currently, no support for definitions that have recursive aggregates");
		return false;
	}
	return true;
}
#endif

