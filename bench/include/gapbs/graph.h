// Copyright (c) 2015, The Regents of the University of California (Regents)
// See LICENSE.txt for license details

#ifndef GRAPH_H_
#define GRAPH_H_

#include <cinttypes>
#include <iostream>
#include <map>

#include <mutex>
#include <stdio.h>
#include <thread>
#include <type_traits>

#include <omp.h>

#include "pvector.h"
#include "aligned_array.h"
#include "util.h"

#include "parallel.h"

#include <assert.h>
#include <memory>

/*
   GAP Benchmark Suite
   Class:  CSRGraph
   Author: Scott Beamer

   Simple container for graph in CSR format
   - Intended to be constructed by a Builder
   - To make weighted, set DestID_ template type to NodeWeight
   - MakeInverse parameter controls whether graph stores its inverse
 */

template <typename FNodeID_, typename WeightT_ = FNodeID_, typename FDestID_ = FNodeID_>
class CSRGraphFlat
{
public:
    AlignedArray<FNodeID_> offsets_;
    AlignedArray<FNodeID_> degrees_;
    AlignedArray<FDestID_> neighbors_;
    AlignedArray<WeightT_> weights_;
    int64_t num_nodes_;
    int64_t num_edges_;
    bool directed_;

    CSRGraphFlat(AlignedArray<FNodeID_> offsets, AlignedArray<FNodeID_> degrees, AlignedArray<FDestID_> neighbors, AlignedArray<WeightT_> weights)
        : offsets_(std::move(offsets)), degrees_(std::move(degrees)), neighbors_(std::move(neighbors)), weights_(std::move(weights)),
          num_nodes_(offsets.size - 1), num_edges_(neighbors.size), directed_(true) {}

    int64_t num_nodes() const
    {
        return num_nodes_;
    }

    int64_t num_edges() const
    {
        return num_edges_;
    }

    auto neighbors_begin(FNodeID_ node) const
    {
        return neighbors_.data + offsets_.data[node];
    }

    auto neighbors_end(FNodeID_ node) const
    {
        return neighbors_.data + offsets_.data[node] + degrees_.data[node];
    }

    auto weights_begin(FNodeID_ node) const
    {
        return weights_.data + offsets_.data[node];
    }

    auto weights_end(FNodeID_ node) const
    {
        return weights_.data + offsets_.data[node] + degrees_.data[node];
    }

    void display(const std::string &name) const
    {
        offsets_.display(name + "_offsets");
        degrees_.display(name + "_degrees");
        neighbors_.display(name + "_neighbors");
        weights_.display(name + "_weights");
    }

    void PrintStats() const
    {
        std::cout << "Graph has " << num_nodes_ << " nodes and " << num_edges_
                  << " ";
        if (!directed_)
            std::cout << "un";
        std::cout << "directed edges for degree: ";
        std::cout << num_edges_ / num_nodes_;

        // Calculate and output the total size in megabytes
        size_t total_size =
            (num_nodes_ * sizeof(FNodeID_) + (num_nodes_ + 1) * sizeof(FNodeID_) +
             num_edges_ * sizeof(FDestID_)) /
            (1024 * 1024);
        std::cout << " Estimated size: " << total_size << " MB" << std::endl;
    }

    void PrintTopology() const
    {
        for (FNodeID_ i = 0; i < num_nodes_; i++)
        {
            std::cout << i << ": ";
            for (auto it = neighbors_begin(i); it != neighbors_end(i); ++it)
            {
                std::cout << *it << " ";
            }
            std::cout << std::endl;
        }
    }
};


// Used to hold node & weight, with another node it makes a weighted edge
template <typename NodeID_, typename WeightT_> struct NodeWeight
{
    NodeID_ v;
    WeightT_ w;
    NodeWeight() {}
    NodeWeight(NodeID_ v) : v(v), w(1) {}
    NodeWeight(NodeID_ v, WeightT_ w) : v(v), w(w) {}

    bool operator<(const NodeWeight &rhs) const
    {
        return v == rhs.v ? w < rhs.w : v < rhs.v;
    }

    // doesn't check WeightT_s, needed to remove duplicate edges
    bool operator==(const NodeWeight &rhs) const
    {
        return v == rhs.v;
    }

    // doesn't check WeightT_s, needed to remove self edges
    bool operator==(const NodeID_ &rhs) const
    {
        return v == rhs;
    }

    operator NodeID_()
    {
        return v;
    }
};

template <typename NodeID_, typename WeightT_>
std::ostream &operator<<(std::ostream &os,
                         const NodeWeight<NodeID_, WeightT_> &nw)
{
    os << nw.v << " " << nw.w;
    return os;
}

template <typename NodeID_, typename WeightT_>
std::istream &operator>>(std::istream &is, NodeWeight<NodeID_, WeightT_> &nw)
{
    is >> nw.v >> nw.w;
    return is;
}

// Syntactic sugar for an edge
template <typename SrcT, typename DstT = SrcT> struct EdgePair
{
    SrcT u;
    DstT v;

    EdgePair() {}

    EdgePair(SrcT u, DstT v) : u(u), v(v) {}

    bool operator<(const EdgePair &rhs) const
    {
        return u == rhs.u ? v < rhs.v : u < rhs.u;
    }

    bool operator==(const EdgePair &rhs) const
    {
        return (u == rhs.u) && (v == rhs.v);
    }
};

// SG = serialized graph, these types are for writing graph to file
typedef int32_t SGID;
typedef EdgePair<SGID> SGEdge;
typedef int64_t SGOffset;

template <class NodeID_, class DestID_ = NodeID_, bool MakeInverse = true, class WeightT_ = NodeID_, class FNodeID_ = NodeID_, class FDestID_ = NodeID_>
class CSRGraph
{
    // Used for *non-negative* offsets within a neighborhood
    typedef std::make_unsigned<std::ptrdiff_t>::type OffsetT;

    // Used to access neighbors of vertex, basically sugar for iterators
    class Neighborhood
    {
        NodeID_ n_;
        DestID_ **g_index_;
        OffsetT start_offset_;

    public:
        Neighborhood(NodeID_ n, DestID_ **g_index, OffsetT start_offset)
            : n_(n), g_index_(g_index), start_offset_(0)
        {
            OffsetT max_offset = end() - begin();
            start_offset_ = std::min(start_offset, max_offset);
        }
        typedef DestID_ *iterator;
        iterator begin()
        {
            return g_index_[n_] + start_offset_;
        }
        iterator end()
        {
            return g_index_[n_ + 1];
        }

        bool contains(DestID_ vertex)
        {
            if constexpr (!std::is_same<NodeID_, DestID_>::value)
            {
                return __gnu_parallel::find_if(
                           begin(), end(), [&](const DestID_ & neighbor)
                {
                    return static_cast<NodeWeight<NodeID_, NodeID_>>(neighbor)
                           .v ==
                           static_cast<NodeWeight<NodeID_, NodeID_>>(vertex).v;
                }) != end();
            }
            else
            {
                return __gnu_parallel::find(begin(), end(), vertex) != end();
            }
        }
    };

    void ReleaseResources()
    {
        if (out_index_ != nullptr)
            delete[] out_index_;
        if (out_neighbors_ != nullptr)
            delete[] out_neighbors_;

        if (directed_)
        {
            if (in_index_ != nullptr)
                delete[] in_index_;
            if (in_neighbors_ != nullptr)
                delete[] in_neighbors_;
        }

        if (org_ids_ != nullptr)
            delete[] org_ids_;
    }

public:
    CSRGraph()
        : directed_(false), num_nodes_(-1), num_edges_(-1), out_index_(nullptr),
          out_neighbors_(nullptr), in_index_(nullptr), in_neighbors_(nullptr),
          org_ids_(nullptr) {}

    CSRGraph(int64_t num_nodes, DestID_ **index, DestID_ *neighs)
        : directed_(false), num_nodes_(num_nodes), out_index_(index),
          out_neighbors_(neighs), in_index_(index), in_neighbors_(neighs)
    {
        num_edges_ = (out_index_[num_nodes_] - out_index_[0]) / 2;
        initialize_org_ids();
    }

    CSRGraph(int64_t num_nodes, DestID_ **out_index, DestID_ *out_neighs,
             DestID_ **in_index, DestID_ *in_neighs)
        : directed_(true), num_nodes_(num_nodes), out_index_(out_index),
          out_neighbors_(out_neighs), in_index_(in_index),
          in_neighbors_(in_neighs)
    {
        num_edges_ = out_index_[num_nodes_] - out_index_[0];
        initialize_org_ids();
    }

    CSRGraph(CSRGraph &&other)
        : directed_(other.directed_), num_nodes_(other.num_nodes_),
          num_edges_(other.num_edges_), out_index_(other.out_index_),
          out_neighbors_(other.out_neighbors_), in_index_(other.in_index_),
          in_neighbors_(other.in_neighbors_), org_ids_(other.org_ids_)
    {
        other.num_edges_ = -1;
        other.num_nodes_ = -1;
        other.out_index_ = nullptr;
        other.out_neighbors_ = nullptr;
        other.in_index_ = nullptr;
        other.in_neighbors_ = nullptr;
        other.org_ids_ = nullptr;
    }

    ~CSRGraph()
    {
        ReleaseResources();
    }

    CSRGraph &operator=(CSRGraph &&other)
    {
        if (this != &other)
        {
            ReleaseResources();
            directed_ = other.directed_;
            num_edges_ = other.num_edges_;
            num_nodes_ = other.num_nodes_;
            out_index_ = other.out_index_;
            out_neighbors_ = other.out_neighbors_;
            in_index_ = other.in_index_;
            in_neighbors_ = other.in_neighbors_;
            org_ids_ = other.org_ids_;
            other.num_edges_ = -1;
            other.num_nodes_ = -1;
            other.out_index_ = nullptr;
            other.out_neighbors_ = nullptr;
            other.in_index_ = nullptr;
            other.in_neighbors_ = nullptr;
            other.org_ids_ = nullptr;
        }
        return *this;
    }

    bool directed() const
    {
        return directed_;
    }

    bool is_weighted() const
    {
        return (!std::is_same<NodeID_, DestID_>::value);
    }

    int64_t num_nodes() const
    {
        return num_nodes_;
    }

    int64_t num_edges() const
    {
        return num_edges_;
    }

    int64_t num_edges_directed() const
    {
        return directed_ ? num_edges_ : 2 * num_edges_;
    }

    int64_t out_degree(NodeID_ v) const
    {
        return out_index_[v + 1] - out_index_[v];
    }

    int64_t in_degree(NodeID_ v) const
    {
        static_assert(MakeInverse, "Graph inversion disabled but reading inverse");
        return in_index_[v + 1] - in_index_[v];
    }

    Neighborhood out_neigh(NodeID_ n, OffsetT start_offset = 0) const
    {
        return Neighborhood(n, out_index_, start_offset);
    }

    Neighborhood in_neigh(NodeID_ n, OffsetT start_offset = 0) const
    {
        static_assert(MakeInverse, "Graph inversion disabled but reading inverse");
        return Neighborhood(n, in_index_, start_offset);
    }

    void PrintStats() const
    {
        std::cout << "Graph has " << num_nodes_ << " nodes and " << num_edges_
                  << " ";
        if (!directed_)
            std::cout << "un";
        std::cout << "directed edges for degree: ";
        std::cout << num_edges_ / num_nodes_;

        // Calculate and output the total size in megabytes
        size_t total_size =
            (num_nodes_ * sizeof(NodeID_) + (num_nodes_ + 1) * sizeof(NodeID_) +
             num_edges_ * sizeof(NodeID_)) /
            (1024 * 1024);
        std::cout << " Estimated size: " << total_size << " MB" << std::endl;
    }

    void PrintTopology() const
    {
        for (NodeID_ i = 0; i < num_nodes_; i++)
        {
            std::cout << i << ": ";
            for (DestID_ j : out_neigh(i))
            {
                // if (is_weighted())
                //   std::cout << static_cast<DestID_>(j).v << " ";
                // else
                std::cout << j << " ";
            }
            std::cout << std::endl;
        }
    }

    static DestID_ **GenIndex(const pvector<SGOffset> &offsets, DestID_ *neighs)
    {
        NodeID_ length = offsets.size();
        DestID_ **index = new DestID_ *[length];
        #pragma omp parallel for
        for (NodeID_ n = 0; n < length; n++)
            index[n] = neighs + offsets[n];
        return index;
    }

    pvector<SGOffset> VertexOffsets(bool in_graph = false) const
    {
        pvector<SGOffset> offsets(num_nodes_ + 1);
        for (NodeID_ n = 0; n < num_nodes_ + 1; n++)
            if (in_graph)
                offsets[n] = in_index_[n] - in_index_[0];
            else
                offsets[n] = out_index_[n] - out_index_[0];
        return offsets;
    }

    Range<NodeID_> vertices() const
    {
        return Range<NodeID_>(num_nodes());
    }

public:
    NodeID_ *get_org_ids() const
    {
        return org_ids_;
    }

    // Function to initialize org_ids_
    void initialize_org_ids()
    {
        if (num_nodes_ == -1)
        {
            if (org_ids_ != nullptr)
                delete[] org_ids_;
            org_ids_ = nullptr;
        }
        else
        {
            org_ids_ = new NodeID_[num_nodes_];
            #pragma omp parallel for
            for (NodeID_ n = 0; n < num_nodes_; n++)
            {
                org_ids_[n] = n;
            }
        }
    }

    void update_org_ids(const pvector<NodeID_> &new_ids)
    {
        if (num_nodes_ == -1)
        {
            org_ids_ = nullptr;
        }
        else
        {
            #pragma omp parallel for
            for (NodeID_ n = 0; n < num_nodes_; n++)
            {
                org_ids_[n] = new_ids[org_ids_[n]];
            }
        }
    }

    void copy_org_ids(const NodeID_ *new_org_ids)
    {
        if (org_ids_ != nullptr)
            delete[] org_ids_;
        org_ids_ = new NodeID_[num_nodes_];
        #pragma omp parallel for
        for (NodeID_ n = 0; n < num_nodes_; n++)
        {
            org_ids_[n] = new_org_ids[n];
        }
    }

    NodeID_ get_org_id(const NodeID_ ref_id) const
    {
        NodeID_ org_id = 0;
        if (num_nodes_ == -1 || ref_id > num_nodes_)
        {
            return ref_id;
        }
        else
        {
            org_id = org_ids_[ref_id];
        }
        return org_id;
    }

    void PrintTopologyOriginal() const
    {
        NodeID_ *org_ids_inv_ = new NodeID_[num_nodes_];
        #pragma omp parallel for
        for (NodeID_ n = 0; n < num_nodes_; n++)
        {
            org_ids_inv_[org_ids_[n]] = n;
        }

        for (NodeID_ i = 0; i < num_nodes_; i++)
        {
            std::cout << org_ids_inv_[i] << ": ";
            for (DestID_ j : out_neigh(i))
            {
                std::cout << org_ids_inv_[j] << " ";
            }
            std::cout << std::endl;
        }

        delete[] org_ids_inv_;
    }


    CSRGraphFlat<FNodeID_, WeightT_, FDestID_> flattenGraphOut(size_t alignment = 4096) const
    {
        AlignedArray<FNodeID_> degrees(num_nodes(), alignment, 0);
        AlignedArray<FNodeID_> offsets(num_nodes() + 1, alignment, 0);
        AlignedArray<FDestID_> neighbors(num_edges_directed(), alignment, 0);
        AlignedArray<WeightT_> weights(num_edges_directed(), alignment, 0);

        // Calculate degrees using the difference of indices
        #pragma omp parallel for
        for (NodeID_ i = 0; i < num_nodes(); ++i)
        {
            degrees.data[i] = out_index_[i + 1] - out_index_[i];
        }

        // Calculate offsets (prefix sum of degrees)
        #pragma omp parallel for
        for (NodeID_ i = 1; i <= num_nodes(); ++i)
        {
            offsets.data[i] = out_index_[i] - out_index_[0];
        }

        // Directly copy neighbors and weights
        #pragma omp parallel for
        for (NodeID_ i = 0; i < num_nodes(); ++i)
        {
            NodeID_ offset = offsets.data[i];
            NodeID_ length = degrees.data[i];
            if constexpr (!std::is_same<NodeID_, DestID_>::value)
            {
                std::transform(out_index_[i], out_index_[i] + length, neighbors.data + offset, [](const DestID_ & neighbor)
                {
                    return static_cast<const NodeWeight<NodeID_, typename NodeWeight<NodeID_, WeightT_>::value> &>(neighbor).v;
                });
                std::transform(out_index_[i], out_index_[i] + length, weights.data + offset, [](const DestID_ & neighbor)
                {
                    return static_cast<const NodeWeight<NodeID_, typename NodeWeight<NodeID_, WeightT_>::value> &>(neighbor).w;
                });
            }
            else
            {
                std::copy(out_index_[i], out_index_[i] + length, neighbors.data + offset);
                std::fill(weights.data + offset, weights.data + offset + length, NodeWeight<NodeID_, WeightT_>(0, 1).w); // Assuming weights are zero for same type
            }
        }

        return CSRGraphFlat<FNodeID_, WeightT_, FDestID_>(std::move(offsets), std::move(degrees), std::move(neighbors), std::move(weights));
    }

private:
    bool directed_;
    int64_t num_nodes_;
    int64_t num_edges_;
    DestID_ **out_index_;
    DestID_ *out_neighbors_;
    DestID_ **in_index_;
    DestID_ *in_neighbors_;
    NodeID_ *org_ids_ = nullptr;
};

#endif // GRAPH_H_