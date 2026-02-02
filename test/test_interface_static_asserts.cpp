#include <kaspan/graph/balanced_slice_part.hpp>
#include <kaspan/graph/block_cyclic_part.hpp>
#include <kaspan/graph/cyclic_part.hpp>
#include <kaspan/graph/explicit_continuous_part.hpp>
#include <kaspan/graph/explicit_sorted_part.hpp>
#include <kaspan/graph/single_part.hpp>
#include <kaspan/graph/trivial_slice_part.hpp>
#include <kaspan/graph/graph_part.hpp>
#include <kaspan/graph/bidi_graph_part.hpp>
#include <kaspan/graph/concept.hpp>

using namespace kaspan;

static_assert(part_concept<balanced_slice_part>);
static_assert(part_view_concept<balanced_slice_part_view>);
static_assert(part_concept<block_cyclic_part>);
static_assert(part_view_concept<block_cyclic_part_view>);
static_assert(part_concept<cyclic_part>);
static_assert(part_view_concept<cyclic_part_view>);
static_assert(part_concept<explicit_continuous_part>);
static_assert(part_view_concept<explicit_continuous_part_view>);
static_assert(part_concept<explicit_sorted_part>);
static_assert(part_view_concept<explicit_sorted_part_view>);
static_assert(part_concept<single_part>);
static_assert(part_view_concept<single_part_view>);
static_assert(part_concept<trivial_slice_part>);
static_assert(part_view_concept<trivial_slice_part_view>);

static_assert(graph_part_concept<graph_part<balanced_slice_part>>);
static_assert(graph_part_view_concept<graph_part_view<balanced_slice_part_view>>);
static_assert(graph_part_concept<bidi_graph_part<balanced_slice_part>>);
static_assert(graph_part_view_concept<bidi_graph_part_view<balanced_slice_part_view>>);

#include <kaspan/graph/graph.hpp>
#include <kaspan/graph/bidi_graph.hpp>

static_assert(graph_part_concept<graph>);
static_assert(graph_part_view_concept<graph_view>);
static_assert(graph_part_concept<bidi_graph>);
static_assert(graph_part_view_concept<bidi_graph_view>);

int main() { return 0; }
