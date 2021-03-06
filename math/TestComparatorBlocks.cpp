// Copyright (c) 2015-2015 Tony Kirke
// Copyright (c) 2016-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <cstdint>
#include <cmath>
#include <iostream>

static const size_t NUM_POINTS = 13;

template <typename Type>
void testComparatorTmpl(const double val, const std::string op_string)
{
    auto dtype = Pothos::DType(typeid(Type));
    std::cout << "Testing comparator with type " << dtype.toString() << ", value " << val << std::endl;

    auto feeder0 = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    auto feeder1 = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    auto comp = Pothos::BlockRegistry::make("/comms/comparator", dtype, op_string);
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", "char");

    //load the feeder
    auto buffIn0 = Pothos::BufferChunk(typeid(Type), NUM_POINTS);
    auto pIn0 = buffIn0.as<Type *>();
    auto buffIn1 = Pothos::BufferChunk(typeid(Type), NUM_POINTS);
    auto pIn1 = buffIn1.as<Type *>();

    for (size_t i = 0; i < buffIn0.elements(); i++)
    {
        pIn0[i] = Type(i);
        pIn1[i] = Type(val);
    }
    
    feeder0.call("feedBuffer", buffIn0);
    feeder1.call("feedBuffer", buffIn1);

    //run the topology
    {
        Pothos::Topology topology;
        topology.connect(feeder0, 0, comp, 0);
        topology.connect(feeder1, 0, comp, 1);
        topology.connect(comp, 0, collector, 0);
        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.01));
    }

    //check the collector
    Pothos::BufferChunk buffOut = collector.call("getBuffer");
    POTHOS_TEST_EQUAL(buffOut.length, NUM_POINTS*sizeof(char));
    auto pOut = buffOut.as<const char *>();
    for (size_t i = 0; i < NUM_POINTS; i++)
    {
      char expected;
      if (op_string == ">")
        expected = (pIn0[i] > pIn1[i]);
      else if (op_string == "<")
        expected = (pIn0[i] < pIn1[i]);
      else if (op_string == "<=")
        expected = (pIn0[i] <= pIn1[i]);
      else if (op_string == ">=")
        expected = (pIn0[i] >= pIn1[i]);
      else if (op_string == "==")
        expected = (pIn0[i] == pIn1[i]);
      else if (op_string == "!=")
        expected = (pIn0[i] != pIn1[i]);
      POTHOS_TEST_EQUAL(pOut[i], expected);
    }
}

void test_comp(const std::string& op) 
{
  for (size_t i = 0; i <= 4; i++)
    {
      const double factor = i/2.0-1.0;
      testComparatorTmpl<double>(factor,op);
      testComparatorTmpl<float>(factor,op);
      testComparatorTmpl<int64_t>(factor,op);
      testComparatorTmpl<int32_t>(factor,op);
      testComparatorTmpl<int16_t>(factor,op);
      testComparatorTmpl<int8_t>(factor,op);
    }
}
POTHOS_TEST_BLOCK("/comms/tests", test_comparator)
{
  test_comp(">");
  test_comp(">=");
  test_comp("<");
  test_comp("<=");
  test_comp("==");
  test_comp("!=");
}
