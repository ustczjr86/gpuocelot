/*! \file TestPTXAssembly.h
	\author Gregory Diamos <gregory.diamos@gatech.edu>
	\date Tuesday May 11, 2010
	\brief The header file for the TestPTXAssembly class.
*/

#ifndef TEST_PTX_ASSEMBLY_H_INCLUDED
#define TEST_PTX_ASSEMBLY_H_INCLUDED

#include <hydrazine/interface/Test.h>
#include <vector>

namespace test
{
	/*! \brief The idea here is to define a test harness for a large number 
		of PTX unit tests.
	*/
	class TestPTXAssembly : public Test
	{
		public:
			/*! \brief Possible data types */
			enum DataType
			{
				I8,
				I16,
				I32,
				I64,
				FP32,
				FP64
			};
		
			/*! \brief A vector of data types */
			typedef std::vector<DataType> TypeVector;

			/*! \brief A pointer to a reference function */
			typedef void (*ReferenceFunction)(void* output, void* input);
			
			/*! \brief A class for representing a single test */
			class TestHandle
			{
				public:
					std::string name;
					ReferenceFunction reference;
					std::string ptx;
					TypeVector inputTypes;
					TypeVector outputTypes;
					unsigned int threads;
					unsigned int ctas;
			};
			
			/*! \brief A list of tests to perform */
			typedef std::vector<TestHandle> TestVector;
	
		private:
			/*! \brief Total amount of time to spend on tests in seconds */
			hydrazine::Timer::Second _timeLimit;
			/*! \brief The list of tests to run */
			TestVector _tests;
			/*! \brief The number of tolerable failures */
			unsigned int _tolerableFailures;
			/*! \brief The PTX device count */
			unsigned int _devices;
			
		public:
			/*! \brief Get the size of a data type in bytes */
			static unsigned int bytes(DataType t);
		
		private:
			/*! \brief Perform a single unit test */
			bool _doOneTest(const TestHandle& test, unsigned int seed);
		
		public:
			/*! \brief Constructor */
			TestPTXAssembly(hydrazine::Timer::Second limit = 10.0, 
				unsigned int tolerableFailures = 0);
			
		public:
			/*! \brief Add a test, ptx function name should be 'test' */
			void add(const std::string& name,
				ReferenceFunction function, const std::string& ptx, 
				const TypeVector& out, const TypeVector& in, 
				unsigned int threads, unsigned int ctas);
				
			/*! \brief Run the current set of tests, abort on the first error */
			bool doTest();
	};
}

int main(int argc, char** argv);

#endif
