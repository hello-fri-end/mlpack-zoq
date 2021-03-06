/**
 * @file tests/convolutional_network_test.cpp
 * @author Marcus Edel
 * @author Abhinav Moudgil
 *
 * Tests the convolutional neural network.
 *
 * mlpack is free software; you may redistribute it and/or modify it under the
 * terms of the 3-clause BSD license.  You should have received a copy of the
 * 3-clause BSD license along with mlpack.  If not, see
 * http://www.opensource.org/licenses/BSD-3-Clause for more information.
 */
#include <mlpack/core.hpp>

#include <mlpack/methods/ann/layer/layer.hpp>
#include <mlpack/methods/ann/ffn.hpp>
#include <mlpack/methods/ann/init_rules/gaussian_init.hpp>

#include <ensmallen.hpp>

#include "serialization.hpp"
#include "catch.hpp"
#include "test_catch_tools.hpp"

using namespace mlpack;
using namespace mlpack::ann;

// network1 should be allocated with `new`, and trained on some data.
template<typename MatType = arma::mat, typename ModelType>
void CheckCopyFunction(ModelType* network1,
                       MatType& trainData,
                       MatType& trainLabels,
                       const size_t maxEpochs)
{
  ens::RMSProp opt(0.01, 1, 0.88, 1e-8, maxEpochs * trainData.n_cols, -1);
  network1->Train(trainData, trainLabels, opt);

  arma::mat predictions1;
  network1->Predict(trainData, predictions1);
  FFN<> network2;
  network2 = *network1;
  delete network1;

  // Deallocating all of network1's memory, so that network2 does not use any
  // of that memory.
  arma::mat predictions2;
  network2.Predict(trainData, predictions2);
  CheckMatrices(predictions1, predictions2);
}

// network1 should be allocated with `new`, and trained on some data.
template<typename MatType = arma::mat, typename ModelType>
void CheckMoveFunction(ModelType* network1,
                       MatType& trainData,
                       MatType& trainLabels,
                       const size_t maxEpochs)
{
  ens::RMSProp opt(0.01, 1, 0.88, 1e-8, maxEpochs * trainData.n_cols, -1);
  network1->Train(trainData, trainLabels, opt);

  arma::mat predictions1;
  network1->Predict(trainData, predictions1);
  FFN<> network2(std::move(*network1));
  delete network1;

  // Deallocating all of network1's memory, so that network2 does not use any
  // of that memory.
  arma::mat predictions2;
  network2.Predict(trainData, predictions2);
  CheckMatrices(predictions1, predictions2);
}

/**
 * Train the vanilla network on a larger dataset.
 */
TEST_CASE("VanillaNetworkTest", "[ConvolutionalNetworkTest]")
{
  arma::mat X;
  X.load("mnist_first250_training_4s_and_9s.arm");

  // Normalize each point since these are images.
  arma::uword nPoints = X.n_cols;
  for (arma::uword i = 0; i < nPoints; ++i)
  {
    X.col(i) /= norm(X.col(i), 2);
  }

  // Build the target matrix.
  arma::mat Y = arma::zeros<arma::mat>(1, nPoints);
  for (size_t i = 0; i < nPoints; ++i)
  {
    if (i < nPoints / 2)
    {
      // Assign label "0" to all samples with digit = 4
      Y(i) = 0;
    }
    else
    {
      // Assign label "1" to all samples with digit = 9
      Y(i) = 1;
    }
  }

  /*
   * Construct a convolutional neural network with a 28x28x1 input layer,
   * 24x24x8 convolution layer, 12x12x8 pooling layer, 8x8x12 convolution layer
   * and a 4x4x12 pooling layer which is fully connected with the output layer.
   * The network structure looks like:
   *
   * Input    Convolution  Pooling      Convolution  Pooling      Output
   * Layer    Layer        Layer        Layer        Layer        Layer
   *
   *          +---+        +---+        +---+        +---+
   *          | +---+      | +---+      | +---+      | +---+
   * +---+    | | +---+    | | +---+    | | +---+    | | +---+    +---+
   * |   |    | | |   |    | | |   |    | | |   |    | | |   |    |   |
   * |   +--> +-+ |   +--> +-+ |   +--> +-+ |   +--> +-+ |   +--> |   |
   * |   |      +-+   |      +-+   |      +-+   |      +-+   |    |   |
   * +---+        +---+        +---+        +---+        +---+    +---+
   */
  // It isn't guaranteed that the network will converge in the specified number
  // of iterations using random weights. If this works 1 of 5 times, I'm fine
  // with that. All I want to know is that the network is able to escape from
  // local minima and to solve the task.
  bool success = false;
  for (size_t trial = 0; trial < 5; ++trial)
  {
    FFN<NegativeLogLikelihood<>, RandomInitialization> model;

    model.Add<Convolution<> >(1, 8, 5, 5, 1, 1, 0, 0, 28, 28);
    model.Add<ReLULayer<> >();
    model.Add<MaxPooling<> >(8, 8, 2, 2);
    model.Add<Convolution<> >(8, 12, 2, 2);
    model.Add<ReLULayer<> >();
    model.Add<MaxPooling<> >(2, 2, 2, 2);
    model.Add<Linear<> >(192, 20);
    model.Add<ReLULayer<> >();
    model.Add<Linear<> >(20, 10);
    model.Add<ReLULayer<> >();
    model.Add<Linear<> >(10, 2);
    model.Add<LogSoftMax<> >();

    // Train for only 8 epochs.
    ens::RMSProp opt(0.001, 1, 0.88, 1e-8, 8 * nPoints, -1);

    double objVal = model.Train(X, Y, opt);

    // Test that objective value returned by FFN::Train() is finite.
    REQUIRE(std::isfinite(objVal) == true);

    arma::mat predictionTemp;
    model.Predict(X, predictionTemp);
    arma::mat prediction = arma::zeros<arma::mat>(1, predictionTemp.n_cols);

    for (size_t i = 0; i < predictionTemp.n_cols; ++i)
    {
      prediction(i) = arma::as_scalar(arma::find(
            arma::max(predictionTemp.col(i)) == predictionTemp.col(i), 1));
    }

    size_t correct = arma::accu(prediction == Y);
    double classificationError = 1 - double(correct) / X.n_cols;
    if (classificationError <= 0.25)
    {
      success = true;
      break;
    }
  }

  REQUIRE(success == true);
}

/**
 * Train the vanilla network on a larger dataset.
 */
TEST_CASE("CheckCopyVanillaNetworkTest", "[ConvolutionalNetworkTest]")
{
  arma::mat X;
  X.load("mnist_first250_training_4s_and_9s.arm");

  // Normalize each point since these are images.
  arma::uword nPoints = X.n_cols;
  for (arma::uword i = 0; i < nPoints; ++i)
  {
    X.col(i) /= norm(X.col(i), 2);
  }

  // Build the target matrix.
  arma::mat Y = arma::zeros<arma::mat>(1, nPoints);
  for (size_t i = 0; i < nPoints; ++i)
  {
    if (i < nPoints / 2)
    {
      // Assign label "0" to all samples with digit = 4
      Y(i) = 0;
    }
    else
    {
      // Assign label "1" to all samples with digit = 9
      Y(i) = 1;
    }
  }

  /*
   * Construct a convolutional neural network with a 28x28x1 input layer,
   * 24x24x8 convolution layer, 12x12x8 pooling layer, 8x8x12 convolution layer
   * and a 4x4x12 pooling layer which is fully connected with the output layer.
   * The network structure looks like:
   *
   * Input    Convolution  Pooling      Convolution  Pooling      Output
   * Layer    Layer        Layer        Layer        Layer        Layer
   *
   *          +---+        +---+        +---+        +---+
   *          | +---+      | +---+      | +---+      | +---+
   * +---+    | | +---+    | | +---+    | | +---+    | | +---+    +---+
   * |   |    | | |   |    | | |   |    | | |   |    | | |   |    |   |
   * |   +--> +-+ |   +--> +-+ |   +--> +-+ |   +--> +-+ |   +--> |   |
   * |   |      +-+   |      +-+   |      +-+   |      +-+   |    |   |
   * +---+        +---+        +---+        +---+        +---+    +---+
   */
  // It isn't guaranteed that the network will converge in the specified number
  // of iterations using random weights. If this works 1 of 5 times, I'm fine
  // with that. All I want to know is that the network is able to escape from
  // local minima and to solve the task.
  FFN<NegativeLogLikelihood<>, RandomInitialization> *model = new FFN<NegativeLogLikelihood<>, RandomInitialization>;

  model->Add<Convolution<> >(1, 8, 5, 5, 1, 1, 0, 0, 28, 28);
  model->Add<ReLULayer<> >();
  model->Add<MaxPooling<> >(8, 8, 2, 2);
  model->Add<Convolution<> >(8, 12, 2, 2);
  model->Add<ReLULayer<> >();
  model->Add<MaxPooling<> >(2, 2, 2, 2);
  model->Add<Linear<> >(192, 20);
  model->Add<ReLULayer<> >();
  model->Add<Linear<> >(20, 10);
  model->Add<ReLULayer<> >();
  model->Add<Linear<> >(10, 2);
  model->Add<LogSoftMax<> >();

  FFN<NegativeLogLikelihood<>, RandomInitialization> *model1 = new FFN<NegativeLogLikelihood<>, RandomInitialization>;

  model1->Add<Convolution<> >(1, 8, 5, 5, 1, 1, 0, 0, 28, 28);
  model1->Add<ReLULayer<> >();
  model1->Add<MaxPooling<> >(8, 8, 2, 2);
  model1->Add<Convolution<> >(8, 12, 2, 2);
  model1->Add<ReLULayer<> >();
  model1->Add<MaxPooling<> >(2, 2, 2, 2);
  model1->Add<Linear<> >(192, 20);
  model1->Add<ReLULayer<> >();
  model1->Add<Linear<> >(20, 10);
  model1->Add<ReLULayer<> >();
  model1->Add<Linear<> >(10, 2);
  model1->Add<LogSoftMax<> >();
  
  // Check whether copy constructor is working or not.
  CheckCopyFunction<>(model, X, Y, 8);

  // Check whether move constructor is working or not.
  CheckMoveFunction<>(model1, X, Y, 8);
}
