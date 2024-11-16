#include <gtest/gtest.h>

#include <boost/mpi.hpp>

#include "mpi/vavilov_v_contrast_enhancement/include/ops_mpi.hpp"

namespace mpi = boost::mpi;

TEST(vavilov_v_contrast_enhancement_mpi, ValidInput) {
  mpi::environment env;
  mpi::communicator world;

  if (world.rank() == 0) {
    auto taskDataPar = std::make_shared<ppc::core::TaskData>();
    std::vector<int> input = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
    taskDataPar->inputs.emplace_back(reinterpret_cast<uint8_t *>(input.data()));
    taskDataPar->inputs_count.emplace_back(input.size());

    vavilov_v_contrast_enhancement_mpi::TestMPITaskParallel testMpiTaskParallel(taskDataPar);
    ASSERT_TRUE(testMpiTaskParallel.validation());
  }
}

TEST(vavilov_v_contrast_enhancement_mpi, CorrectOutputSize) {
  mpi::environment env;
  mpi::communicator world;

  if (world.rank() == 0) {
    auto taskDataPar = std::make_shared<ppc::core::TaskData>();
    std::vector<int> input = {10, 20, 30, 40, 50};

    taskDataPar->inputs.emplace_back(reinterpret_cast<uint8_t *>(input.data()));
    taskDataPar->inputs_count.emplace_back(input.size());
    taskDataPar->outputs.emplace_back(reinterpret_cast<uint8_t *>(input.data()));
    taskDataPar->outputs_count.emplace_back(input.size());

    vavilov_v_contrast_enhancement_mpi::TestMPITaskParallel testMpiTaskParallel(taskDataPar);
    ASSERT_TRUE(testMpiTaskParallel.validation());
  }
}

TEST(vavilov_v_contrast_enhancement_mpi, IncorrectOutputSize) {
  mpi::environment env;
  mpi::communicator world;

  if (world.rank() == 0) {
    auto taskDataPar = std::make_shared<ppc::core::TaskData>();
    std::vector<int> input = {10, 20, 30, 40, 50};

    taskDataPar->inputs.emplace_back(reinterpret_cast<uint8_t *>(input.data()));
    taskDataPar->inputs_count.emplace_back(input.size());
    taskDataPar->outputs.emplace_back(reinterpret_cast<uint8_t *>(input.data()));
    taskDataPar->outputs_count.emplace_back(input.size() - 1);

    vavilov_v_contrast_enhancement_mpi::TestMPITaskParallel testMpiTaskParallel(taskDataPar);
    ASSERT_FALSE(testMpiTaskParallel.validation());
  }
}

TEST(vavilov_v_contrast_enhancement_mpi, NormalContrastEnhancement) {
  mpi::environment env;
  mpi::communicator world;

  auto taskDataPar = std::make_shared<ppc::core::TaskData>();
  std::vector<int> input = {10, 20, 30, 40, 50};
  taskDataPar->inputs_count.emplace_back(input.size());
  taskDataPar->outputs_count.emplace_back(input.size());
  std::vector<int> output(input.size());
  taskDataPar->inputs.emplace_back(reinterpret_cast<uint8_t *>(input.data()));
  taskDataPar->outputs.emplace_back(reinterpret_cast<uint8_t *>(output.data()));

  vavilov_v_contrast_enhancement_mpi::TestMPITaskParallel testMpiTaskParallel(taskDataPar);
  ASSERT_TRUE(testMpiTaskParallel.validation());
  ASSERT_TRUE(testMpiTaskParallel.pre_processing());
  ASSERT_TRUE(testMpiTaskParallel.run());
  ASSERT_TRUE(testMpiTaskParallel.post_processing());

  if (world.rank() == 0) {
    std::vector<int> expected_output = {0, 63, 127, 191, 255};
    EXPECT_EQ(output, expected_output);
  }
}
