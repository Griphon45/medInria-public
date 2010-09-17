#ifndef DCMTKFINDCALLBACK_H
#define DCMTKFINDCALLBACK_H

#ifndef WIN32
#define HAVE_CONFIG_H
#endif

// inc
#include "dcmtk/dcmnet/dfindscu.h"
#include <iostream>
#include <vector>

// fwd
class dcmtkFindDataset;
class dcmtkResultDatasetContainer;
class dcmtkKeyContainer;

/** Custom implementation of FindSCU callback class. This implementation is 
 *  used when no explicit callback is passed by the user, e.g. in the findscu tool.
 */
class dcmtkFindScuCallback: public DcmFindSCUCallback
{
public:
  /**
   * constructor
   * @param extractResponsesToFile if true, C-FIND-RSP datasets will be stored as DICOM files
   * @param cancelAfterNResponses if non-negative, a C-FIND-CANCEL will be issued after the
   * given number of incoming C-FIND-RSP messages
   */
  dcmtkFindScuCallback(OFBool extractResponsesToFile, int cancelAfterNResponses);
  
  /// destructor
  virtual ~dcmtkFindScuCallback(){};

  /** 
   * callback method that is called once for each incoming C-FIND-RSP message.
   * @param request DIMSE command of the original C-FIND request
   * @param responseCount number of current response
   * @param rsp DIMSE command of incoming C-FIND response
   * @param responseIdentifiers dataset of incoming C-FIND response
   */
  virtual void callback(T_DIMSE_C_FindRQ *request, int responseCount, T_DIMSE_C_FindRSP *rsp,
                        DcmDataset *responseIdentifiers);

  /**
   * set a pointer to an dcmtkResultDatasetContainer that will be filled inside the callback function
   */
  void setResultDatasetContainer(dcmtkResultDatasetContainer*  dataCont);


  /**
  * setKeyContainer to look for keys in the result dataset
  * @params: cont pointer to keycontainer
  */
  void setKeyContainer(dcmtkKeyContainer* cont);

private:

   /// if true, C-FIND-RSP datasets will be stored as DICOM files
   OFBool extractResponsesToFile_;

   /// if non-negative, a C-FIND-CANCEL will be issued after the given number of incoming C-FIND-RSP messages
   int cancelAfterNResponses_;

   dcmtkResultDatasetContainer*          m_ds;

   dcmtkKeyContainer*                    m_keyCont;

};

#endif DCMTKFINDCALLBACK_H
