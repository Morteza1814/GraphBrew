// Copyright (c) 2015, The Regents of the University of California (Regents)
// See LICENSE.txt for license details

#ifndef READER_H_
#define READER_H_

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <cstring> // for std::strtok
#include <sys/stat.h> // for stat

#include "pvector.h"
#include "util.h"

/*
   GAP Benchmark Suite
   Class:  Reader
   Author: Scott Beamer

   Given filename, returns an edgelist or the entire graph (if serialized)
   - Intended to be called from Builder
   - Determines file format from the filename's suffix
   - If the input graph is serialized (.sg or .wsg), reads the graph
   directly into the returned graph instance
   - Otherwise, reads the file and returns an edgelist
 */

template <typename NodeID_, typename DestID_ = NodeID_,
          typename WeightT_ = NodeID_, bool invert = true>
class Reader {
typedef EdgePair<NodeID_, DestID_> Edge;
typedef pvector<Edge> EdgeList;
std::string filename_;

public:
explicit Reader(std::string filename) : filename_(filename) {
}

std::string GetSuffix() {
  std::size_t suff_pos = filename_.rfind('.');
  if (suff_pos == std::string::npos) {
    std::cout << "Couldn't find suffix of " << filename_ << std::endl;
    std::exit(-1);
  }
  return filename_.substr(suff_pos);
}

// Function to get the size of a file
std::size_t GetFileSize(const std::string &filename) {
  struct stat stat_buf;
  int rc = stat(filename.c_str(), &stat_buf);
  return rc == 0 ? stat_buf.st_size : -1;
}

// EdgeList ReadInEL(std::ifstream &in) {
//   EdgeList el;
//   NodeID_ u, v;
//   while (in >> u >> v) {
//     el.push_back(Edge(u, v));
//   }
//   return el;
// }

// Assuming Edge, NodeID_, and EdgeList types are defined elsewhere
EdgeList ReadInEL(std::ifstream &in) {
  EdgeList el;
  NodeID_ u, v;
  std::string line;

  while (std::getline(in, line)) {
    // Check if the line is empty or starts with % or #
    if (line.empty() || line[0] == '%' || line[0] == '#')
      continue;

    // Use a stringstream to read node ids from the line
    std::istringstream iss(line);
    if (iss >> u >> v) {
      el.push_back(Edge(u, v));
    }
  }
  return el;
}

// EdgeList ReadInEL(const std::string &filename) {
//   std::ifstream in(filename);
//   if (!in.is_open()) {
//     std::cerr << "Unable to open file: " << filename << std::endl;
//     std::exit(-1);
//   }

//   std::size_t file_size = GetFileSize(filename);
//   if (file_size == static_cast<std::size_t>(-1)) {
//     std::cerr << "Unable to get file size: " << filename << std::endl;
//     std::exit(-1);
//   }

//   const std::size_t average_line_length = 20; // Estimate average line length in bytes
//   std::size_t initial_capacity = file_size / average_line_length;

//   EdgeList el;
//   el.reserve(initial_capacity);

//   std::string buffer;
//   buffer.reserve(1024 * 1024); // Reserve 1 MB buffer for reading

//   while (std::getline(in, buffer)) {
//     // Check if the line is empty or starts with % or #
//     if (buffer.empty() || buffer[0] == '%' || buffer[0] == '#') {
//       continue;
//     }

//     // Tokenize the line
//     char* str = &buffer[0];
//     char* token = std::strtok(str, " \t\n\r");
//     if (!token) continue;
//     NodeID_ u = std::stoi(token);

//     token = std::strtok(nullptr, " \t\n\r");
//     if (!token) continue;
//     NodeID_ v = std::stoi(token);

//     el.emplace_back(Edge(u, v));
//   }

//   el.shrink_to_fit(); // Resize to actual filled size
//   return el;
// }

// EdgeList ReadInWEL(std::ifstream &in) {
//   EdgeList el;
//   NodeID_ u;
//   NodeWeight<NodeID_, WeightT_> v;
//   while (in >> u >> v) {
//     el.push_back(Edge(u, v));
//   }
//   return el;
// }

// Assuming Edge, NodeID_, NodeWeight, and EdgeList types are defined
// elsewhere
EdgeList ReadInWEL(std::ifstream &in) {
  EdgeList el;
  NodeID_ u;
  NodeWeight<NodeID_, WeightT_> v;
  std::string line;

  while (std::getline(in, line)) {
    // Check if the line is empty or starts with % or #
    if (line.empty() || line[0] == '%' || line[0] == '#')
      continue;

    // Use a stringstream to read node id and node weight from the line
    std::istringstream iss(line);
    if (iss >> u >> v) {
      el.push_back(Edge(u, v));
    }
  }
  return el;
}

// Note: converts vertex numbering from 1..N to 0..N-1
EdgeList ReadInGR(std::ifstream &in) {
  EdgeList el;
  char c;
  NodeID_ u;
  NodeWeight<NodeID_, WeightT_> v;
  while (!in.eof()) {
    c = in.peek();
    if (c == 'a') {
      in >> c >> u >> v;
      el.push_back(Edge(u - 1, NodeWeight<NodeID_, WeightT_>(v.v - 1, v.w)));
    } else {
      in.ignore(200, '\n');
    }
  }
  return el;
}

// Note: converts vertex numbering from 1..N to 0..N-1
EdgeList ReadInMetis(std::ifstream &in, bool &needs_weights) {
  EdgeList el;
  NodeID_ num_nodes, num_edges;
  char c;
  std::string line;
  bool read_weights = false;
  while (true) {
    c = in.peek();
    if (c == '%') {
      in.ignore(200, '\n');
    } else {
      std::getline(in, line, '\n');
      std::istringstream header_stream(line);
      header_stream >> num_nodes >> num_edges;
      header_stream >> std::ws;
      if (!header_stream.eof()) {
        int32_t fmt;
        header_stream >> fmt;
        if (fmt == 1) {
          read_weights = true;
        } else if ((fmt != 0) && (fmt != 100)) {
          std::cout << "Do not support METIS fmt type: " << fmt << std::endl;
          std::exit(-20);
        }
      }
      break;
    }
  }
  NodeID_ u = 0;
  while (u < num_nodes) {
    c = in.peek();
    if (c == '%') {
      in.ignore(200, '\n');
    } else {
      std::getline(in, line);
      if (line != "") {
        std::istringstream edge_stream(line);
        if (read_weights) {
          NodeWeight<NodeID_, WeightT_> v;
          while (edge_stream >> v >> std::ws) {
            v.v -= 1;
            el.push_back(Edge(u, v));
          }
        } else {
          NodeID_ v;
          while (edge_stream >> v >> std::ws) {
            el.push_back(Edge(u, v - 1));
          }
        }
      }
      u++;
    }
  }
  needs_weights = !read_weights;
  return el;
}

// Note: converts vertex numbering from 1..N to 0..N-1
// Note: weights casted to type WeightT_
EdgeList ReadInMTX(std::ifstream &in, bool &needs_weights) {
  EdgeList el;
  std::string start, object, format, field, symmetry, line;
  in >> start >> object >> format >> field >> symmetry >> std::ws;
  // std::cout << "Header: " << start << " " << object << " " << format << " " << field << " " << symmetry << std::endl;

  if (start != "%%MatrixMarket") {
    std::cout << ".mtx file did not start with %%MatrixMarket" << std::endl;
    std::exit(-21);
  }
  if ((object != "matrix") || (format != "coordinate")) {
    std::cout << "only allow matrix coordinate format for .mtx" << std::endl;
    std::exit(-22);
  }
  if (field == "complex") {
    std::cout << "do not support complex weights for .mtx" << std::endl;
    std::exit(-23);
  }
  bool read_weights;
  if (field == "pattern") {
    read_weights = false;
  } else if ((field == "real") || (field == "double") || (field == "integer")) {
    read_weights = true;
  } else {
    std::cout << "unrecognized field type for .mtx" << std::endl;
    std::exit(-24);
  }
  bool undirected;
  if (symmetry == "symmetric") {
    undirected = true;
  } else if ((symmetry == "general") || (symmetry == "skew-symmetric")) {
    undirected = false;
  } else {
    std::cout << "unsupported symmetry type for .mtx" << std::endl;
    std::exit(-25);
  }

  // Skip all comment lines
  while (std::getline(in, line)) {
    if (line[0] != '%') {
      break;
    }
  }

  // Read the dimensions and non-zeros line explicitly
  std::istringstream dimensions_stream(line);
  int64_t m = 0, n = 0, nonzeros = 0;
  if (!(dimensions_stream >> m >> n >> nonzeros)) {
    std::cout << "Error parsing matrix dimensions and non-zeros" << std::endl;
    std::exit(-28);
  }

  // std::cout << "Matrix dimensions and nonzeros: " << m << " " << n << " " << nonzeros << std::endl;

  if (m != n) {
    std::cout << m << " " << n << " " << nonzeros << std::endl;
    std::cout << "matrix must be square for .mtx" << std::endl;
    std::exit(-26);
  }

  while (std::getline(in, line)) {
    if (line.empty())
      continue;
    std::istringstream edge_stream(line);
    NodeID_ u;
    edge_stream >> u;
    if (read_weights) {
      NodeWeight<NodeID_, WeightT_> v;
      edge_stream >> v;
      v.v -= 1;
      el.push_back(Edge(u - 1, v));
      if (undirected)
        el.push_back(Edge(v.v, NodeWeight<NodeID_, WeightT_>(u - 1, v.w)));
    } else {
      NodeID_ v;
      edge_stream >> v;
      el.push_back(Edge(u - 1, v - 1));
      if (undirected)
        el.push_back(Edge(v - 1, u - 1));
    }
  }
  needs_weights = !read_weights;
  return el;
}




EdgeList ReadInAstar(std::ifstream &in) {
  EdgeList el;

  // Entry reading utilities
  auto readU = [&]() -> uint32_t {
           union U {
             uint32_t val;
             char bytes[sizeof(uint32_t)];
           };
           U u;
           in.read(u.bytes, sizeof(uint32_t));
           assert(!in.fail());
           return u.val;
         };

  auto readD = [&]() -> double {
           union U {
             double val;
             char bytes[sizeof(double)];
           };
           U u;
           in.read(u.bytes, sizeof(double));
           assert(!in.fail());
           return u.val;
         };

  uint32_t magic = readU();

  if (magic != 0x150842A7) {
    std::cout << "Cannot read astar graph: Magic number mismatch."
              << std::endl;
    std::exit(-1);
  }
  uint32_t numNodes = readU();
  // std::cout << "Reading " << numNodes << " nodes." << std::endl;

  for (uint32_t u = 0; u < numNodes; u++) {
    readD();
    readD();
    uint32_t numNeighbors = readU();

    std::vector<NodeWeight<NodeID_, WeightT_> > neighbors(numNeighbors);

    for (uint32_t j = 0; j < numNeighbors; j++) {
      neighbors[j].v = readU();
    }

    for (uint32_t j = 0; j < numNeighbors; j++) {
      static double EARTH_RADIUS_CM = 637100000.0;
      neighbors[j].w = readD() * EARTH_RADIUS_CM;
    }

    for (auto neighbor : neighbors) {
      el.push_back(Edge(u, neighbor));
    }
  }

  return el;
}

EdgeList ReadFile(bool &needs_weights) {
  Timer t;
  t.Start();
  EdgeList el;
  std::string suffix = GetSuffix();
  std::ifstream file(filename_);
  if (!file.is_open()) {
    std::cout << "Couldn't open file " << filename_ << std::endl;
    std::exit(-2);
  }
  if (suffix == ".el") {
    el = ReadInEL(file);
  } else if (suffix == ".wel") {
    needs_weights = false;
    el = ReadInWEL(file);
  } else if (suffix == ".gr") {
    needs_weights = false;
    el = ReadInGR(file);
  } else if (suffix == ".graph") {
    el = ReadInMetis(file, needs_weights);
  } else if (suffix == ".mtx") {
    el = ReadInMTX(file, needs_weights);
  } else {
    std::cout << "Unrecognized suffix: " << suffix << std::endl;
    std::exit(-3);
  }
  file.close();
  t.Stop();
  PrintTime("Read Time", t.Seconds());
  return el;
}

CSRGraph<NodeID_, DestID_, invert> ReadSerializedGraph() {
  bool weighted = GetSuffix() == ".wsg";
  CSRGraph<NodeID_, DestID_, invert> g_new;
  bool generate_weights = false;
  bool clear_weights = false;
  if (!std::is_same<NodeID_, SGID>::value) {
    std::cout << "serialized graphs only allowed for 32bit" << std::endl;
    std::exit(-5);
  }
  if (!weighted && !std::is_same<NodeID_, DestID_>::value) {
    // std::cout << ".sg not allowed for weighted graphs" << std::endl;
    // std::exit(-5);
    generate_weights = true;
  }
  if (weighted && std::is_same<NodeID_, DestID_>::value) {
    // std::cout << ".wsg only allowed for weighted graphs" << std::endl;
    // std::exit(-5);
    clear_weights = true;
  }
  if (weighted && !std::is_same<WeightT_, SGID>::value) {
    std::cout << ".wsg only allowed for int32_t weights" << std::endl;
    std::exit(-5);
  }
  std::ifstream file(filename_);
  if (!file.is_open()) {
    std::cout << "Couldn't open file " << filename_ << std::endl;
    std::exit(-6);
  }
  Timer t;
  t.Start();
  bool directed;
  SGOffset num_nodes, num_edges;
  DestID_ **index = nullptr, **inv_index = nullptr;
  DestID_ *neighs = nullptr, *inv_neighs = nullptr;
  file.read(reinterpret_cast<char *>(&directed), sizeof(bool));
  file.read(reinterpret_cast<char *>(&num_edges), sizeof(SGOffset));
  file.read(reinterpret_cast<char *>(&num_nodes), sizeof(SGOffset));
  pvector<SGOffset> offsets(num_nodes + 1);
  neighs = new DestID_[num_edges];
  std::streamsize num_index_bytes = (num_nodes + 1) * sizeof(SGOffset);
  std::streamsize num_neigh_bytes = num_edges * sizeof(DestID_);
  std::streamsize num_neigh_bytes_clear =
    num_edges * sizeof(NodeWeight<NodeID_, WeightT_>);
  file.read(reinterpret_cast<char *>(offsets.data()), num_index_bytes);

  if (generate_weights) {
    NodeID_ *temp_neighs = new NodeID_[num_edges];
    std::streamsize temp_num_neigh_bytes = num_edges * sizeof(NodeID_);
    file.read(reinterpret_cast<char *>(temp_neighs), temp_num_neigh_bytes);

#pragma omp parallel for
    for (int i = 0; i < num_edges; ++i) {
      reinterpret_cast<NodeWeight<NodeID_, WeightT_> *>(&neighs[i])->v =
        temp_neighs[i];
      reinterpret_cast<NodeWeight<NodeID_, WeightT_> *>(&neighs[i])->w = 1;
    }
    delete[] temp_neighs;
  } else {
    if (clear_weights) {
      NodeWeight<NodeID_, WeightT_> *temp_neighs_clear =
        new NodeWeight<NodeID_, WeightT_>[num_edges];
      file.read(reinterpret_cast<char *>(temp_neighs_clear),
                num_neigh_bytes_clear);
#pragma omp parallel for
      for (int i = 0; i < num_edges; ++i) {
        neighs[i] = temp_neighs_clear[i].v;
        // cout << temp_neighs_clear[i] << endl;
      }
      delete[] temp_neighs_clear;
    } else {
      file.read(reinterpret_cast<char *>(neighs), num_neigh_bytes);
    }
  }
  index = CSRGraph<NodeID_, DestID_>::GenIndex(offsets, neighs);

  if (directed && invert) {
    inv_neighs = new DestID_[num_edges];
    file.read(reinterpret_cast<char *>(offsets.data()), num_index_bytes);

    if (generate_weights) {
      NodeID_ *temp_inv_neighs = new NodeID_[num_edges];
      std::streamsize temp_num_inv_neighs_bytes = num_edges * sizeof(NodeID_);
      file.read(reinterpret_cast<char *>(temp_inv_neighs),
                temp_num_inv_neighs_bytes);

#pragma omp parallel for
      for (int i = 0; i < num_edges; ++i) {
        reinterpret_cast<NodeWeight<NodeID_, WeightT_> *>(&inv_neighs[i])->v =
          temp_inv_neighs[i];
        reinterpret_cast<NodeWeight<NodeID_, WeightT_> *>(&inv_neighs[i])->w =
          1;
      }
      delete[] temp_inv_neighs;
    } else {
      if (clear_weights) {
        NodeWeight<NodeID_, WeightT_> *temp_inv_neighs_clear =
          new NodeWeight<NodeID_, WeightT_>[num_edges];
        file.read(reinterpret_cast<char *>(temp_inv_neighs_clear),
                  num_neigh_bytes_clear);
#pragma omp parallel for
        for (int i = 0; i < num_edges; ++i) {
          inv_neighs[i] = temp_inv_neighs_clear[i].v;
        }
        delete[] temp_inv_neighs_clear;
      } else {
        file.read(reinterpret_cast<char *>(inv_neighs), num_neigh_bytes);
      }
    }
    inv_index = CSRGraph<NodeID_, DestID_>::GenIndex(offsets, inv_neighs);
  }
  NodeID_ *org_ids = new NodeID_[num_nodes];
  file.read(reinterpret_cast<char *>(org_ids),
            num_nodes * sizeof(NodeID_)); // Read original IDs
  file.close();
  t.Stop();
  PrintTime("Read Time", t.Seconds());
  if (directed)
    g_new = CSRGraph<NodeID_, DestID_, invert>(num_nodes, index, neighs,
                                               inv_index, inv_neighs);
  else
    g_new = CSRGraph<NodeID_, DestID_, invert>(num_nodes, index, neighs);

  g_new.copy_org_ids(org_ids);

  delete[] org_ids;
  return g_new;
}
};

#endif // READER_H_
