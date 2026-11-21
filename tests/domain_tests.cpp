#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "data/JsonMapRepository.h"
#include "data/PngExporter.h"
#include "domain/MapService.h"

using namespace argmap;

namespace {

domain::ArgumentDraft draft(const std::string& text, domain::Side side) {
    domain::ArgumentDraft d;
    d.text = text;
    d.side = side;
    d.weight = 4;
    d.color = "#123456";
    d.speaker = domain::Speaker::Two;
    return d;
}

} // namespace

int main() {
    domain::MapService service;
    service.setTitle("Test debate");

    const std::string p1 = service.addArgument(draft("Taxes are fair", domain::Side::Proposition));
    const std::string o1 = service.addArgument(draft("Taxes hurt growth", domain::Side::Opposition));
    assert(service.map().arguments.size() == 2);

    const std::string link = service.addCounter(o1, p1);
    assert(!link.empty());
    assert(service.stats().oppositionCounters == 1);

    bool threw = false;
    try {
        service.addCounter(o1, p1); // duplicate
    } catch (const domain::DomainError&) {
        threw = true;
    }
    assert(threw);

    threw = false;
    try {
        service.addCounter(p1, p1); // self-link
    } catch (const domain::DomainError&) {
        threw = true;
    }
    assert(threw);

    service.moveArgument(p1, 0.31, 0.42);
    const domain::Layout layout = service.computeLayout();
    assert(layout.nodes.size() == 2);
    assert(layout.links.size() == 1);
    assert(layout.nodes.front().x > 0.3 && layout.nodes.front().x < 0.32);

    // Persistence round-trip.
    data::JsonMapRepository repository;
    const std::string path = "/tmp/argmap_domain_test.argmap";
    std::string error;
    assert(repository.save(service.map(), path, &error));
    domain::DebateMap loaded;
    assert(repository.load(path, &loaded, &error));
    assert(loaded.title == "Test debate");
    assert(loaded.arguments.size() == 2);
    assert(loaded.counters.size() == 1);
    assert(loaded.arguments[0].color == "#123456");
    std::remove(path.c_str());

    // PNG export writes a valid signature and IHDR chunk.
    data::PngExporter png;
    const std::string imagePath = "/tmp/argmap_domain_test.png";
    std::vector<unsigned char> pixels(4 * 4 * 3, 200);
    assert(png.writePng(imagePath, pixels.data(), 4, 4, 3, &error));
    std::ifstream image(imagePath, std::ios::binary);
    std::string bytes((std::istreambuf_iterator<char>(image)),
                      std::istreambuf_iterator<char>());
    assert(bytes.size() > 33);
    assert(static_cast<unsigned char>(bytes[0]) == 137);
    assert(bytes.substr(1, 3) == "PNG");
    std::remove(imagePath.c_str());

    return 0;
}
