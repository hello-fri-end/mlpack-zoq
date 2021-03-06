/**
 * @file size_checks.hpp
 * @author Kirill Mishchenko
 * @author Bisakh Mondal
 *
 * Utility for checking same size & same dimensionality.
 *
 * mlpack is free software; you may redistribute it and/or modify it under the
 * terms of the 3-clause BSD license.  You should have received a copy of the
 * 3-clause BSD license along with mlpack.  If not, see
 * http://www.opensource.org/licenses/BSD-3-Clause for more information.
 */
#ifndef MLPACK_UTIL_SIZE_CHECKS_HPP
#define MLPACK_UTIL_SIZE_CHECKS_HPP


namespace mlpack {
namespace util {

/**
 * Check for if the given data points & labels have same size.
 *
 * @param data data.
 * @param labels Labels.
 * @param callerDescription A description of the caller that can be used for
 *     error generation.
 * @param addInfo Name to use for labels for precise error generation. Default
 *     is "labels"; for example, "weights" could also be used.
 */
template<typename DataType, typename LabelsType>
inline void CheckSameSizes(const DataType& data,
                           const LabelsType& label,
                           const std::string& callerDescription,
                           const std::string& addInfo = "labels")
{
  if (data.n_cols != label.n_cols)
  {
    std::ostringstream oss;
    oss << callerDescription << ": number of points (" << data.n_cols << ") "
        << "does not match number of " << addInfo << " (" << label.n_cols
        << ")!" << std::endl;
    throw std::invalid_argument(oss.str());
  }
}

/**
 * An overload of CheckSameSizes() where the size to be checked is known
 * previously. The second parameter is of type unsigned int.
 */
template<typename DataType>
inline void CheckSameSizes(const DataType& data,
                           const size_t& size,
                           const std::string& callerDescription,
                           const std::string& addInfo = "labels")
{
  if (data.n_cols != size)
  {
    std::ostringstream oss;
    oss << callerDescription << ": number of points (" << data.n_cols << ") "
        << "does not match number of " << addInfo << " (" << size << ")!"
        << std::endl;
    throw std::invalid_argument(oss.str());
  }
}


/**
 * Check for if the given dataset dimension matches with the model's.
 *
 * @param data dataset.
 * @param dimension Dimension of the model.
 * @param callerDescription A description of the caller that can be used for
 *     error generation.
 * @param addInfo Name to use for dataset for precise error generation. Default
 *     is "dataset"; for example, "weights" could also be used.
 */
template<typename DataType, typename DimType>
inline void CheckSameDimensionality(const DataType& data,
                                    const DimType& dimension,
                                    const std::string& callerDescription,
                                    const std::string& addInfo = "dataset")
{
  if (data.n_rows != dimension.n_rows)
  {
    std::ostringstream oss;
    oss << callerDescription << ": dimensionality of " << addInfo << " ("
        << data.n_rows << ") is not equal to the dimensionality of the model"
        " (" << dimension.n_rows << ")!";

    throw std::invalid_argument(oss.str());
  }
}

/**
 * An overload of CheckSameDimensionality() where the dimension to be checked
 * is known second param is unsigned long int.
 */
template<typename DataType>
inline void CheckSameDimensionality(const DataType& data,
                                    const size_t& dimension,
                                    const std::string& callerDescription,
                                    const std::string& addInfo = "dataset")
{
  if (data.n_rows != dimension)
  {
    std::ostringstream oss;
    oss << callerDescription << ": dimensionality of " << addInfo << " ("
        << data.n_rows << ") is not equal to the dimensionality of the model"
        " (" << dimension << ")!";
    throw std::invalid_argument(oss.str());
  }
}

} // namespace util
} // namespace mlpack

#endif
