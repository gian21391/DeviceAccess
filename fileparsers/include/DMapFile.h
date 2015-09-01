/**
 *      @file           DMapFile.h
 *      @author         Adam Piotrowski <adam.piotrowski@desy.de>
 *      @version        1.0
 *      @brief          Provides storage object for devices descriptions                
 */
#ifndef MTCA4U_DMAP_FILE_H
#define	MTCA4U_DMAP_FILE_H

#include <string>
#include <stdint.h>
#include <vector>
#include <iostream>
#include <list>
#include <boost/shared_ptr.hpp>

namespace mtca4u{

/**
 *      @brief  Provides container to store information about devices described in DMAP file. 
 *      
 *      Stores detailed information about all devices described in DMAP file. 
 *      Provides functionality like searching for detailed information about 
 *      device and checking for DMAP file correctness. Does not perform DMAP file parsing
 *
 */
class DMapFile {
    friend class DMapFilesParser;
public:

    /**
     * @brief  Stores information about one device 
     */
    class dRegisterInfo {
    public:
        std::string dev_name; /**< logical name of the device*/
        std::string dev_file; /**< name of dev file (in direcotry /dev)*/
        std::string map_file_name; /**< name of the MAP file storing information about PCIe registers mapping*/
        std::string dmap_file_name; /**< name of the DMAP file*/        
        uint32_t dmap_file_line_nr; /**< line number in DMAP file storing listed above information*/
    public:
        /**
         * Default class constructor
         */
        dRegisterInfo();

	/** Convenience function to extract the device file name and the map file name as one object (a pair).
	 *  This is all the information needed to open a devMap opject. As std::pair and std::string 
	 *  are standard objects no dependency between dmapFile and the devMap object is introduced, in contrast
	 *  to passing a dRegisterInfo to devMap.
	 *  The function name is a bit lengthy to avoid confusion between device name (logical name) and
	 *  device file name (name of the device in the /dev directory). The latter is the .first argument of 
	 *  the pair.
	 */
	std::pair<std::string, std::string> getDeviceFileAndMapFileName() const;

        friend std::ostream& operator<<(std::ostream &os, const dRegisterInfo& de);
    };

    typedef std::vector<dRegisterInfo>::iterator iterator;
    typedef std::vector<dRegisterInfo>::const_iterator const_iterator;
    /**
     * @brief  Stores information about errors and warnings
     *       
     * Stores information about all errors and warnings detected during DMAP file correctness check
     */
    class errorList {
        friend class DMapFile;
        friend class DMapFilesParser;
    public:

        /**
         * @brief  Stores detailed information about one error or warning
         *       
         * Stores detailed information about one error or warnings detected during DMAP file correctness check
         */
        class errorElem {
        public:

            /**
             * @brief  Defines available types of detected problems
             */
            typedef enum {
                NONUNIQUE_DEVICE_NAME /**< Names of two devices are the same - treated as critical error */
            } DMAP_FILE_ERR;

            /**
             * @brief  Defines available classes of detected problems
             * 
             * Posibble values are ERROR or WARNING - used if user wants to limit number of reported 
             * problems only to critical errors or wants to report all detected problems (errors and warnings)
             */
            typedef enum {
                ERROR, /**< Critical error was detected */
                WARNING /**< Non-critical error was detected */
            } TYPE;
            DMapFile::dRegisterInfo err_dev_1; /**< Detailed information about first device that generate error or warning */
            DMapFile::dRegisterInfo err_dev_2; /**< Detailed information about second device that generate error or warning */
            DMAP_FILE_ERR err_type; /**< Type of detected problem */
            TYPE type; /**< Class of detected problem - ERROR or WARNING*/

        public:
            /**
             * Creates obiect that describe one detected error or warning
             * 
             * @param info_type type of detected problem - ERROR or WARNING
             * @param e_type detailed type of detected problem 
             * @param dev_1 detailed information about first device that generate problem
             * @param dev_2 detailed information about second device that generate problem
             */
            errorElem(TYPE info_type, DMAP_FILE_ERR e_type, const DMapFile::dRegisterInfo &dev_1, const DMapFile::dRegisterInfo &dev_2);
            friend std::ostream& operator<<(std::ostream &os, const TYPE& me);
            friend std::ostream& operator<<(std::ostream &os, const errorElem& me);
        };
        std::list<errorElem> errors; /**< Lists of errors or warnings detected during MAP file correctness checking*/

    public:
        friend std::ostream& operator<<(std::ostream &os, const errorList& me);

    private:
        /**
         * Delete all elements from error list
         */
        void clear();
        /**
         * Insert new error on error list
         * @param elem object describing detected error or warning
         */
        void insert(const errorElem& elem);
    };
    /**
     * @brief Checks logical correctness of DMAP file. 
     * 
     * Checks if names in DMAP file are unique. Errors are not reported
     * if two devices with the same name have the same parameters. Checks only syntactic correctness of 
     * data stored in DMAP file. Syntax and lexical analizys are performed by DMAP file parser. 
     * 
     * @param err list of detected errors
     * @param level level of checking - if ERROR is selected only errors will be reported, if WARNING is selected
     *          errors and warning will be reported 
     * @return false if error or warning was detected, otherwise true
     * 
     * @snippet test-libmap.cpp DMAP file correctness checking
     */
    bool check(errorList &err, errorList::errorElem::TYPE level);

    friend std::ostream& operator<<(std::ostream &os, const DMapFile& me);
    /**
     * @brief Returns information about specified device
     * 
     * @throw exDmapFile [exLibMap::EX_NO_DEVICE_IN_DMAP_FILE] - no device with specified name 
     * @param dev_name name of the device
     * @param value detailed information about device taken from DMAP file
     * 
     * @snippet test-libmap.cpp DMAP getting info
     */
    void getDeviceInfo(const std::string& dev_name, dRegisterInfo &value);
    /**
     * @brief Returns number of records in DMAP file
     *
     * @return number of records in DMAP file
     */
    size_t getdmapFileSize();
    /**
     * @brief Return iterator to first device described in DMAP file
     * 
     * @return iterator to first element in DMAP file
     * 
     * @snippet test-libmap.cpp DMAP iterating throught all devices
     */
    iterator begin();
    const_iterator begin() const;
    /**
     * @brief Return iterator to element after last one in DMAP file
     * 
     * @return iterator to element after last one in DMAP file
     * 
     * @snippet test-libmap.cpp DMAP iterating throught all devices
     */
    iterator end();
    const_iterator end() const;

private:
    std::vector<dRegisterInfo> dmap_file_elems; /**< vector storing parsed contents of DMAP file*/
    std::string dmap_file_name; /**< name of DMAP file*/

public:
    /**
     * @brief Constructor
     * 
     * Initialize DMAP file name stored into object but does not perform DMAP file parsing
     * 
     * @param file_name name of DMAP file
     */
    DMapFile(const std::string &file_name);
    /**
     * @brief Insert new element read from DMAP file
     * @param elem element describing detailes of one device taken from DMAP file
     */
    void insert(const dRegisterInfo &elem);
};
/**
 * @typedef Introduce specialisation of shared_pointer template for pointers to mapFile object as a ptrdmapFile
 */
typedef boost::shared_ptr<DMapFile> ptrdmapFile;

}//namespace mtca4u

#endif	/* MTCA4U_DMAP_FILE_H */
