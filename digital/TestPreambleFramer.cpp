// Copyright (c) 2015-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Testing.hpp>
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <iostream>
#include <complex>

POTHOS_TEST_BLOCK("/comms/tests", test_preamble_framer)
{
    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", "unsigned char");
    auto framer = Pothos::BlockRegistry::make("/comms/preamble_framer");
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", "unsigned char");

    const std::vector<unsigned char> preamble{0, 1, 1, 1, 1, 0};
    size_t testLength = 40;
    size_t startIndex = 5;
    size_t endIndex = 33;
    size_t paddingSize = 13;

    framer.call("setPreamble", preamble);
    framer.call("setFrameStartId", "myFrameStart");
    framer.call("setFrameEndId", "myFrameEnd");
    framer.call("setPaddingSize", paddingSize);

    //load feeder blocks
    auto b0 = Pothos::BufferChunk(typeid(unsigned char), testLength);
    auto p0 = b0.as<unsigned char *>();
    for (size_t i = 0; i < testLength; i++) p0[i] = i % 2;
    feeder.call("feedBuffer", b0);
    feeder.call("feedLabel", Pothos::Label("myFrameStart", Pothos::Object(), startIndex));
    feeder.call("feedLabel", Pothos::Label("myFrameEnd", Pothos::Object(), endIndex));

    //run the topology
    {
        Pothos::Topology topology;
        topology.connect(feeder, 0, framer, 0);
        topology.connect(framer, 0, collector, 0);
        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive());
    }

    //check the collector buffer matches input with preamble inserted
    Pothos::BufferChunk buff = collector.call("getBuffer");
    POTHOS_TEST_EQUAL(buff.elements(), testLength + preamble.size() + paddingSize);
    auto pb = buff.as<const unsigned char *>();
    POTHOS_TEST_EQUALA(pb, p0, startIndex); //check data before frame
    POTHOS_TEST_EQUALA(pb+startIndex, preamble.data(), preamble.size()); //check preamble insertion
    POTHOS_TEST_EQUALA(pb+startIndex+preamble.size(), p0+startIndex, endIndex-startIndex+1); //check frame
    POTHOS_TEST_EQUALA(pb+endIndex+preamble.size()+paddingSize+1, p0+endIndex+1, testLength-(endIndex+1)); //check data after frame

    //check the label positions
    std::vector<Pothos::Label> labels = collector.call("getLabels");
    POTHOS_TEST_EQUAL(labels.size(), 2);
    POTHOS_TEST_EQUAL(labels[0].id, "myFrameStart");
    POTHOS_TEST_EQUAL(labels[0].index, startIndex);
    POTHOS_TEST_EQUAL(labels[1].id, "myFrameEnd");
    POTHOS_TEST_EQUAL(labels[1].index, endIndex+preamble.size()+paddingSize);
}
