
/*
 Copyright (C) 2003 Neil Firth
 Copyright (C) 2002, 2003 Ferdinando Ametrano
 Copyright (C) 2002, 2003 Sad Rejeb
 Copyright (C) 2000, 2001, 2002, 2003 RiskMap srl

 This file is part of QuantLib, a free-software/open-source library
 for financial quantitative analysts and developers - http://quantlib.org/

 QuantLib is free software: you can redistribute it and/or modify it under the
 terms of the QuantLib license.  You should have received a copy of the
 license along with this program; if not, please email quantlib-dev@lists.sf.net
 The license is also available online at http://quantlib.org/html/license.html

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the license for more details.
*/

/*! \file mcdigitalengine.hpp
    \brief digital option Monte Carlo engine
*/

#ifndef quantlib_digital_mc_engine_hpp
#define quantlib_digital_mc_engine_hpp

#include <ql/exercise.hpp>
#include <ql/handle.hpp>
#include <ql/termstructure.hpp>
#include <ql/voltermstructure.hpp>
#include <ql/MonteCarlo/mctraits.hpp>
#include <ql/PricingEngines/Vanilla/mcvanillaengine.hpp>

namespace QuantLib {

    //! Pricing engine for digital options using Monte Carlo simulation
    /*! Uses the Brownian Bridge correction for the barrier found in
        <i>
        Going to Extremes: Correcting Simulation Bias in Exotic
        Option Valuation - D.R. Beaglehole, P.H. Dybvig and G. Zhou
        Financial Analysts Journal; Jan/Feb 1997; 53, 1. pg. 62-68
        </i>
        and
        <i>
        Simulating path-dependent options: A new approach - 
        M. El Babsiri and G. Noel
        Journal of Derivatives; Winter 1998; 6, 2; pg. 65-83
        </i>
    */
    template<class RNG = PseudoRandom, class S = Statistics>
    class MCDigitalEngine : public MCVanillaEngine<RNG,S> {
      public:
        typedef typename MCVanillaEngine<RNG,S>::path_generator_type
            path_generator_type;
        typedef typename MCVanillaEngine<RNG,S>::path_pricer_type
            path_pricer_type;
        typedef typename MCVanillaEngine<RNG,S>::stats_type
            stats_type;
        // the uniform generator to use in path generation and
        // path correction
//        typedef typename RNG::ursg_type my_sequence_type;

        // constructor
        MCDigitalEngine(Size maxTimeStepsPerYear,
                        bool antitheticVariate = false,
                        bool controlVariate = false,
                        Size requiredSamples = Null<int>(),
                        double requiredTolerance = Null<double>(),
                        Size maxSamples = Null<int>(),
                        long seed = 0);

        void calculate() const;
      protected:

        // McSimulation implementation
        TimeGrid timeGrid() const;
//        Handle<path_generator_type> pathGenerator() const;
        Handle<path_pricer_type> pathPricer() const;

        // data members
        //my_sequence_type uniformGenerator_;
//        Size maxTimeStepsPerYear_;
//        Size requiredSamples_, maxSamples_;
//        double requiredTolerance_;
//        long seed_;
    };

    class DigitalPathPricer : public PathPricer<Path> {
      public:
        DigitalPathPricer(const Handle<CashOrNothingPayoff>& payoff,
                          const Handle<AmericanExercise>& exercise,
                          double underlying,
                          const RelinkableHandle<TermStructure>& riskFreeTS,
                          const Handle<DiffusionProcess>& diffProcess,
                          const PseudoRandom::ursg_type& sequenceGen);
        double operator()(const Path& path) const;
      private:
        Handle<CashOrNothingPayoff> payoff_;
        Handle<AmericanExercise> exercise_;
        double underlying_;
        Handle<DiffusionProcess> diffProcess_;
        PseudoRandom::ursg_type sequenceGen_;
    };



    // template definitions

    template<class RNG, class S>
    MCDigitalEngine<RNG,S>::MCDigitalEngine(Size maxTimeStepsPerYear,
                                          bool antitheticVariate,
                                          bool controlVariate,
                                          Size requiredSamples,
                                          double requiredTolerance,
                                          Size maxSamples,
                                          long seed)
    : MCVanillaEngine<RNG,S>(maxTimeStepsPerYear,
                             antitheticVariate,
                             controlVariate,
                             requiredSamples,
                             requiredTolerance,
                             maxSamples,
                             seed) {}

/*
    template<class RNG, class S>
    Handle<QL_TYPENAME MCDigitalEngine<RNG,S>::path_generator_type>
    MCDigitalEngine<RNG,S>::pathGenerator() const {

        Handle<DiffusionProcess> bs(new
            BlackScholesProcess(
                arguments_.blackScholesProcess->riskFreeTS,
                arguments_.blackScholesProcess->dividendTS,
                arguments_.blackScholesProcess->volTS,
                arguments_.blackScholesProcess->underlying));

        TimeGrid grid = timeGrid();

        typename RNG::rsg_type gen =
            RNG::make_sequence_generator(grid.size()-1, seed_);

        return Handle<path_generator_type>(
            new path_generator_type(bs, grid, gen));

    }
*/

    template <class RNG, class S>
    Handle<QL_TYPENAME MCDigitalEngine<RNG,S>::path_pricer_type>
    MCDigitalEngine<RNG,S>::pathPricer() const {

        #if defined(HAVE_BOOST)
        Handle<CashOrNothingPayoff> payoff =
            boost::dynamic_pointer_cast<CashOrNothingPayoff>(arguments_.payoff);
        QL_REQUIRE(payoff,
                   "MCDigitalEngine: wrong payoff given");
        #else
        Handle<CashOrNothingPayoff> payoff = arguments_.payoff;
        #endif

        #if defined(HAVE_BOOST)
        Handle<AmericanExercise> exercise =
            boost::dynamic_pointer_cast<AmericanExercise>(arguments_.exercise);
        QL_REQUIRE(exercise,
                   "MCDigitalEngine: wrong exercise given");
        #else
        Handle<AmericanExercise> exercise = arguments_.exercise;
        #endif

        TimeGrid grid = timeGrid();
        PseudoRandom::ursg_type sequenceGen(grid.size()-1, 
                                            PseudoRandom::urng_type(76));

        return Handle<MCDigitalEngine<RNG,S>::path_pricer_type>(new
          DigitalPathPricer(
            payoff,
            exercise,
            arguments_.blackScholesProcess->stateVariable->value(),
            arguments_.blackScholesProcess->riskFreeTS,
            Handle<DiffusionProcess>(new
                BlackScholesProcess(
                    arguments_.blackScholesProcess->riskFreeTS,
                    arguments_.blackScholesProcess->dividendTS,
                    arguments_.blackScholesProcess->volTS,
                    arguments_.blackScholesProcess->stateVariable->value())),
            sequenceGen));
    }


    template <class RNG, class S>
    inline
    TimeGrid MCDigitalEngine<RNG,S>::timeGrid() const {
        Time t = arguments_.blackScholesProcess->riskFreeTS->dayCounter().yearFraction(
            arguments_.blackScholesProcess->riskFreeTS->referenceDate(),
            arguments_.exercise->lastDate());
        return TimeGrid(t, Size(QL_MAX(t * maxTimeStepsPerYear_, 1.0)));
    }

    template<class RNG, class S>
    void MCDigitalEngine<RNG,S>::calculate() const {

        QL_REQUIRE(requiredTolerance_ != Null<double>() ||
                   int(requiredSamples_) != Null<int>(),
                   "MCDigitalEngine::calculate: "
                   "neither tolerance nor number of samples set");

        //! Initialize the one-factor Monte Carlo
        if (controlVariate_) {

            Handle<path_pricer_type> controlPP = controlPathPricer();
            QL_REQUIRE(!IsNull(controlPP),
                       "MCDigitalEngine::calculate() : "
                       "engine does not provide "
                       "control variation path pricer");

            Handle<PricingEngine> controlPE = controlPricingEngine();

            QL_REQUIRE(!IsNull(controlPE),
                       "MCDigitalEngine::calculate() : "
                       "engine does not provide "
                       "control variation pricing engine");
        } else {
            mcModel_ =
                Handle<MonteCarloModel<SingleAsset<RNG>, S> >(
                    new MonteCarloModel<SingleAsset<RNG>, S>(
                        pathGenerator(), pathPricer(), S(),
                        antitheticVariate_));
        }

        if (requiredTolerance_ != Null<double>()) {
            if (int(maxSamples_) != Null<int>())
                value(requiredTolerance_, maxSamples_);
            else
                value(requiredTolerance_);
        } else {
            valueWithSamples(requiredSamples_);
        }

        results_.value = mcModel_->sampleAccumulator().mean();
        if (RNG::allowsErrorEstimate)
            results_.errorEstimate =
                mcModel_->sampleAccumulator().errorEstimate();
    }

}


#endif
