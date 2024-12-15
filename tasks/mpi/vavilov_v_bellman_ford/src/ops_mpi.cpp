#include "mpi/vavilov_v_bellman_ford/include/ops_mpi.hpp"


void vavilov_v_bellman_ford_mpi::TestMPITaskSequential::CRS(const int* matrix) {
  row_ptr.push_back(0);
  for (int i = 0; i < vertices_; ++i) {
    for (int j = 0; j < vertices_; ++j) {
      if (matrix[i * vertices_ + j] != 0) {
        weights_.push_back(input_matrix[i * V + j]);
        col_indices_.push_back(j);
      }
    }
    row_offsets_.push_back(values.size());
  }
};

bool vavilov_v_bellman_ford_mpi::TestMPITaskSequential::pre_processing() {
  internal_order_test();

  vertices_ = taskData->inputs_count[0];
  edges_count_ = taskData->inputs_count[1];
  source_ = taskData->inputs_count[2];

  auto* matrix = reinterpret_cast<int*>(taskData->inputs[0]);

  CRS(matrix);

  distances_.resize(vertices_, INT_MAX);
  distances_[source_] = 0;

  return true;
}

bool vavilov_v_bellman_ford_mpi::TestMPITaskSequential::validation() {
  internal_order_test();

  return (!taskData->inputs.empty());
}

bool vavilov_v_bellman_ford_mpi::TestMPITaskSequential::run() {
  internal_order_test();
  for (int u = 0; u < vertices_; ++u) {
    for (int j = row_offsets_[u]; j < row_offsets_[u + 1]; ++j) {
      int v = col_indices_[j];
      int weight = weights_[j];
      if (distances_[u] != INT_MAX && distances_[u] + weight < distances_[v]) {
        distances_[v] = distances_[u] + weight;
      }
    }
  }

  for (int u = 0; u < vertices_; ++u) {
    for (int j = row_offsets_[u]; j < row_offsets_[u + 1]; ++j) {
      int v = col_indices_[j];
      int weight = weights_[j];
      if (distances_[u] != INT_MAX && distances_[u] + weight < distances_[v]) {
        return false;  // Negative weight cycle detected
      }
    }
  }

  return true;
}

bool vavilov_v_bellman_ford_mpi::TestMPITaskSequential::post_processing() {
  internal_order_test();

  std::copy(distances_.begin(), distances_.end(), reinterpret_cast<int*>(taskData->outputs[0]));
  return true;
}

void vavilov_v_bellman_ford_mpi::TestMPITaskParallel::CRS(const int* matrix) {
  row_ptr.push_back(0);
  for (int i = 0; i < vertices_; ++i) {
    for (int j = 0; j < vertices_; ++j) {
      if (matrix[i * vertices_ + j] != 0) {
        weights_.push_back(input_matrix[i * V + j]);
        col_indices_.push_back(j);
      }
    }
    row_offsets_.push_back(values.size());
  }
};

bool vavilov_v_bellman_ford_mpi::TestMPITaskParallel::pre_processing() {
  internal_order_test();
  vertices_ = taskData->inputs_count[0];
  edges_count_ = taskData->inputs_count[1];
  source_ = taskData->inputs_count[2];

  auto* matrix = reinterpret_cast<int*>(taskData->inputs[0]);

  CRS(matrix);

  boost::mpi::broadcast(world, row_offsets_, 0);
  boost::mpi::broadcast(world, col_indices_, 0);
  boost::mpi::broadcast(world, weights_, 0);

  distances_.resize(vertices_, INT_MAX);
  distances_[source_] = 0;

  return true;
}

bool vavilov_v_bellman_ford_mpi::TestMPITaskParallel::validation() {
  internal_order_test();

  if (world.rank() == 0) {
    return (!taskData->inputs.empty());
  }
  return true;
}

bool vavilov_v_bellman_ford_mpi::TestMPITaskParallel::run() {
  internal_order_test();

  int local_start = world.rank() * (edges_count_ / world.size()) + std::min(world.rank(), edges_count_ % world.size());
  int local_end = local_start + (edges_count_ / world.size()) + (world.rank() < edges_count_ % world.size() ? 1 : 0);

  for (int i = 0; i < vertices_ - 1; ++i) {
    std::vector<int> local_distances = distances_;
    bool local_changed = false;

    for (int u = local_start; u < local_end; ++u) {
      for (int j = row_offsets_[u]; j < row_offsets_[u + 1]; ++j) {
        int v = col_indices_[j];
        int weight = weights_[j];
        if (distances_[u] != INT_MAX && distances_[u] + weight < local_distances[v]) {
          local_distances[v] = distances_[u] + weight;
          local_changed = true;
        }
      }
    }

    boost::mpi::all_reduce(world, local_distances.data(), vertices_, distances_.data(), [](int a, int b) {
      return (a == INT_MAX) ? b : (b == INT_MAX) ? a : std::min(a, b);
    });

    bool global_changed = boost::mpi::all_reduce(world, local_changed, std::logical_or<bool>());
    if (!global_changed) break;
  }

  bool has_negative_cycle = false;
  for (int u = local_start; u < local_end; ++u) {
    for (int j = row_offsets_[u]; j < row_offsets_[u + 1]; ++j) {
      int v = col_indices_[j];
      int weight = weights_[j];
      if (distances_[u] != INT_MAX && distances_[u] + weight < distances_[v]) {
        has_negative_cycle = true;
        break;
      }
    }
  }

  has_negative_cycle = boost::mpi::all_reduce(world, has_negative_cycle, std::logical_or<bool>());
  return !has_negative_cycle;
}

bool vavilov_v_bellman_ford_mpi::TestMPITaskParallel::post_processing() {
  internal_order_test();

  if (world.rank() == 0) {
    std::copy(distances_.begin(), distances_.end(), reinterpret_cast<int*>(taskData->outputs[0]));
  }
  return true;
}