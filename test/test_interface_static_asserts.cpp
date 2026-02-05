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

static_assert(part_c<balanced_slice_part>);
static_assert(part_view_c<balanced_slice_part_view>);
static_assert(part_c<block_cyclic_part>);
static_assert(part_view_c<block_cyclic_part_view>);
static_assert(part_c<cyclic_part>);
static_assert(part_view_c<cyclic_part_view>);
static_assert(part_c<explicit_continuous_part>);
static_assert(part_view_c<explicit_continuous_part_view>);
static_assert(part_c<explicit_sorted_part>);
static_assert(part_view_c<explicit_sorted_part_view>);
static_assert(part_c<single_part>);
static_assert(part_view_c<single_part_view>);
static_assert(part_c<trivial_slice_part>);
static_assert(part_view_c<trivial_slice_part_view>);

static_assert(graph_part_c<graph_part<balanced_slice_part>>);
static_assert(graph_part_view_c<graph_part_view<balanced_slice_part_view>>);
static_assert(graph_part_c<bidi_graph_part<balanced_slice_part>>);
static_assert(graph_part_view_c<bidi_graph_part_view<balanced_slice_part_view>>);

#include <kaspan/graph/graph.hpp>
#include <kaspan/graph/bidi_graph.hpp>

static_assert(graph_part_c<graph>);
static_assert(graph_part_view_c<graph_view>);
static_assert(graph_part_c<bidi_graph>);
static_assert(graph_part_view_c<bidi_graph_view>);

int main() { return 0; }
