

/*! @file ConfigFile.h
 *  @brief Configuration File Reader Specification
 *
 *  This file contains the specification of the FileReader class.
 */

#ifndef __CONFIGURATION_FILE_H__
#define __CONFIGURATION_FILE_H__

namespace Configuration {

  /*!
   *
   *  This structure contains the configuration parameter
   *  name and value pair.
   */
  typedef struct {
    const char *option;
    const char *value;
  } Options_t;


  /*! @class FileReader
   *
   *  This is the specification of the FileReader base class.
   *  All FileReader implementations inherit from this class.
   */
  class FileReader {

  public:

    /*!
     *  This method constructs a FileReader instance.
     *
     *  @param[in] options is the set of options
     */
    FileReader(
      Options_t *options
    );

    /*!
     *  This method destructs a FileReader instance.
     */
    virtual ~FileReader();

    /*!
     *  This method processes the configuratino information from the input
     *  @a file.
     *
     *  @param[in] file is the coverage file to process
     *
     *  @return Returns TRUE if the method succeeded and FALSE if it failed.
     */
    virtual bool processFile(
      const char* const     file
    );

    bool setOption(
      const char* const option,
      const char* const value
    );

    const char *getOption(
      const char* const option
    );

    void printOptions(void);

  private:
    /*!
     *  This method processes the configuratino information from the input
     *  @a file.
     *
     *  @param[in] option is the name of the option
     *  @param[in] value is the associated value
     *
     *  @return Returns TRUE if the method succeeded and FALSE if it failed.
     */
    Options_t *options_m;

  };

}
#endif
