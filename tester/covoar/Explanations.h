/*! @file Explanations.h
 *  @brief Explanations Specification
 *
 *  This file contains the specification of the Explanations class.
 */

#ifndef __EXPLANATIONS_H__
#define __EXPLANATIONS_H__

#include <map>
#include <stdint.h>
#include <string>
#include <vector>

namespace Coverage {

  /*! @class Explanation
   *
   *  This class defines the information that comprises an explanation
   *  of an uncovered range or branch.
   */
  class Explanation {

  public:

    /*!
     *  This member variable contains the starting line number of
     *  the uncovered range or branch.
     */
    std::string startingPoint;

    /*!
     *  This member variable contains the classification of
     *  the explanation.
     */
    std::string classification;

    /*!
     *  This member variable contains the multi-line explanation text.
     */
    std::vector<std::string> explanation;

    /*!
     *  This member variable indicates whether this explanation was
     *  used during analysis.
     */
    bool found;

    /*!
     *  This method constructs an Explanation instance.
     */
    Explanation() {found = false;}

    /*!
     *  This method destructs an Explanation instance.
     */
    ~Explanation() {}
  };

  /*! @class Explanations
   *
   *  This class defines a set of Explanation instances.
   */
  class Explanations {

  public:

    /*!
     *  This member variable contains a list of Explanation instances.
     */
    std::map<std::string, Explanation> set;

    /*!
     *  This method constructs an Explanations instance.
     */
    Explanations();

    /*!
     *  This method destructs an Explanations instance.
     */
    ~Explanations();

    /*!
     *  This methods loads the explanation information from the
     *  specified file.
     *
     *  @param[in] explanations specifies the file name containing
     *             the explanation information
     */
    void load(
      const std::string& explanations
    );

    /*!
     *  This method returns the explanation associated with the
     *  specified starting line number.
     *
     *  @param[in] start specifies the starting line number for
     *             which to search
     */
    const Explanation *lookupExplanation(
      const std::string& start
    );

    /*!
     *  This method writes a file that contains a list of any
     *  explanations that were not looked up.
     *
     *  @param[in] fileName specifies the name of the file to write
     */
    void writeNotFound(
      const std::string& fileName
    );

  };

}

#endif
