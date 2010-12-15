/*! \file BasicBlockInstrumentor.cpp
	\date Monday November 15, 2010
	\author Naila Farooqui <naila@cc.gatech.edu>
	\brief The source file for the BasicBlockInstrumentor class.
*/

#ifndef BASIC_BLOCK_INSTRUMENTOR_CPP_INCLUDED
#define BASIC_BLOCK_INSTRUMENTOR_CPP_INCLUDED

#include <ocelot/analysis/interface/BasicBlockInstrumentor.h>

#include <ocelot/cuda/interface/cuda_runtime.h>

#include <ocelot/analysis/interface/BasicBlockInstrumentationPass.h>
#include <ocelot/analysis/interface/BasicBlockExecutionCountPass.h>
#include <ocelot/ir/interface/Module.h>

#include <hydrazine/implementation/ArgumentParser.h>
#include <hydrazine/implementation/string.h>
#include <hydrazine/implementation/debug.h>
#include <hydrazine/implementation/Exception.h>

#include <fstream>

using namespace hydrazine;

namespace analysis
{

    void BasicBlockInstrumentor::analyze(ir::Module &module) {
        basicBlocks = 0;
        if(!kernelName.empty()){
            basicBlocks = module.kernels().find(kernelName)->second->dfg()->size();
        }
        else {
            for (ir::Module::KernelMap::const_iterator kernel = module.kernels().begin(); 
		        kernel != module.kernels().end(); ++kernel) {
                basicBlocks += (kernel->second)->dfg()->size();
            }
        }
    }

    void BasicBlockInstrumentor::initialize() {
        
        counter = 0;
        cudaMalloc((void **) &counter, basicBlocks * threadBlocks * threads * sizeof(size_t));
        cudaMemset( counter, 0, basicBlocks * threadBlocks * threads * sizeof( size_t ));
        
        cudaMemcpyToSymbol(((BasicBlockInstrumentationPass *)pass)->basicBlockCounterBase().c_str(), &counter, sizeof(*counter), 0, cudaMemcpyHostToDevice);
    }

    analysis::Pass *BasicBlockInstrumentor::createPass() {
        return new analysis::BasicBlockExecutionCountPass;           
    }

    size_t* BasicBlockInstrumentor::extractResults(std::ostream *out) {

        size_t *info = new size_t[basicBlocks * threads * threadBlocks];
        if(counter) {
            cudaMemcpy(info, counter, basicBlocks * threads * threadBlocks * sizeof( size_t ), cudaMemcpyDeviceToHost);
            cudaFree(counter);
        }

        *out << "\n\n" << kernelName << ":\n";
        *out << "\n--------------- " << description << " ---------------\n\n";

        _kernelProfile.basicBlockExecutionCountMap.clear();

        size_t i = 0;
        size_t j = 0;
        size_t k = 0;
        
	basicBlocks = basicBlocks - 2;

        for(k = 0; k < threadBlocks; k++) {
            for(i = 0; i < basicBlocks; i++) {
                for(j = 0; j < threads; j++) {
                    _kernelProfile.basicBlockExecutionCountMap[i] += info[(i * threads) + (k * basicBlocks * threads) + j];
                }
            }   
        }

        for(j = 0; j < basicBlocks; j++) {
            *out << "Total Execution Count for Basic Block " << j << ": " << _kernelProfile.basicBlockExecutionCountMap[j] << std::endl;
        }
        
        return info;
    }

    void BasicBlockInstrumentor::emitJSON(size_t *info) 
    {
    	if(info == NULL)
            return;
		
		json::Object *basicBlockStat = new json::Object;
		json::Object *column;
		json::Number *valueBBC;
		
		/* device name */
		column = new json::Object;
		basicBlockStat->dictionary["device"] = new json::String(deviceName);
		
		/* per thread BB execution count */
		column = new json::Object;
		basicBlockStat->dictionary["per_thread_BB_execution_count2"] = column;
		for(unsigned int i=0; i<threadBlocks; i++)
		{
			for(unsigned int j=0; j<threads; j++)
			{
				for(unsigned int k=0; k<basicBlocks; k++)
				{
					std::stringstream thread;
					thread << i << "." << j << "." << k;
					valueBBC = new json::Number(int(info[i*threads*basicBlocks+j*basicBlocks+k]));
					column->dictionary[thread.str()] = valueBBC;
				}
			}
		}
	
		/* per cta bbc */ 
		column = new json::Object;
		basicBlockStat->dictionary["per_CTA_BB_execution_count"] = column;
		for(unsigned int i=0; i<basicBlocks; i++)
		{
			for(unsigned int j=0; j<threadBlocks; j++)
			{
				std::stringstream thread;
				thread << j << "." << i;
				int sum = 0;
				for(unsigned int k=0; k<threads; k++)
				{
					sum += info[j*threads*basicBlocks+k*basicBlocks+i];
				}
				valueBBC = new json::Number(sum);
				column->dictionary[thread.str()] = valueBBC;
			}
		}
	
		/* total bbc */ 
		column = new json::Object;
		basicBlockStat->dictionary["BB_execution_count"] = column;
		for(unsigned int i=0; i<basicBlocks; i++)
		{
			std::stringstream thread;
			thread << i;
			int sum = 0;
			for(unsigned int j=0; j<threads*threadBlocks; j++)
			{
				sum += info[j*basicBlocks+i];
			}
			valueBBC = new json::Number(sum);
			column->dictionary[thread.str()] = valueBBC;
		}
		
		jsonEmitter("basicBlockCount", basicBlockStat);
    }

    BasicBlockInstrumentor::BasicBlockInstrumentor() : description("Basic Block Execution Count Per Thread") {

    }

}

#endif