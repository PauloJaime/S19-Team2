#include <boost/filesystem.hpp>
#include <iostream>

#include "SpiceUsr.h"

#include "spyce.hpp"
#include "spyce_exceptions.hpp"

#define SPYCE_OBJECTS_MAX 100
#define NAIF_NAME_MAX     33
#define DATE_STR_MAX      81

/**
 * Internal Functions
 **/
void check_spice_errors() {
    if(!failed_c()) return;

    char mesg[26] = {0};

    getmsg_c("SHORT", 26, mesg);

    reset_c();
    if       (eqstr_c(mesg, "SPICE(EMPTYSTRING)")) {
        throw InvalidArgumentException("Empty String");
    } else if(eqstr_c(mesg, "SPICE(NOSUCHFILE)")) {
        throw FileNotFoundException();
    } else if(eqstr_c(mesg, "SPICE(BADFILETYPE)")) {
        throw InvalidFileException("Bad File Type");
    } else if(eqstr_c(mesg, "SPICE(BADARCHTYPE)")) {
        throw InvalidFileException("Bad Architecture Type");
    } else if(eqstr_c(mesg, "SPICE(INVALIDFORMAT)")) {
        throw InvalidFileException("Invalid Format");
    } else if(eqstr_c(mesg, "SPICE(IDCODENOTFOUND)")) {
        throw IDNotFoundException();
    } else if(eqstr_c(mesg, "SPICE(SPKINSUFFDATA)")) {
        throw InsufficientDataException();
    } else if(eqstr_c(mesg, "SPICE(INVALIDTIMESTRING)")) {
        throw InvalidArgumentException("Invalid Time String");
    } else if(eqstr_c(mesg, "SPICE(INVALIDTIMEFORMAT)")) {
        throw InvalidArgumentException("Invalid Time Format");
    } else {
        //any other errors throw and InternalException
        throw InternalException(mesg);
    }
}
/**
 * Frame class
 **/
Frame::Frame(SpiceDouble *frame) {
    this->x  = frame[0];
    this->y  = frame[1];
    this->z  = frame[2];
    this->dx = frame[3];
    this->dy = frame[4];
    this->dz = frame[5];
}

Frame::Frame() {
    this->x  = 0;
    this->y  = 0;
    this->z  = 0;
    this->dx = 0;
    this->dy = 0;
    this->dz = 0;
}

/**
 * Spyce
 **/
void spyce_init() {
    // Error Handling
    //https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/req/error.html
    erract_c("SET", 0, (SpiceChar *)"RETURN"); // disable "exit on error"
    errprt_c("SET", 0, (SpiceChar *)"NONE");   // don't print anything
    errdev_c("SET", 0, (SpiceChar *)"NULL");   // in case something is printed, send it nowhere.
}

//Helper Functions
int spyce_str_to_id(std::string naif_id) {
    int  id_code;
    SpiceBoolean found;

    bodn2c_c(naif_id.c_str(), &id_code, &found);
    check_spice_errors();

    if(!found)
        throw IDNotFoundException();

    return id_code;
}

std::string spyce_id_to_str(int naif_id) {
    char naif_name[NAIF_NAME_MAX] = {0};
    SpiceBoolean found;

    bodc2n_c(naif_id, NAIF_NAME_MAX, naif_name, &found);
    check_spice_errors();

    if(!found)
        throw IDNotFoundException();

    return std::string(naif_name);
}

double spyce_utc_to_et(std::string date) {
    //acceptable date formats:
    //https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/cspice/utc2et_c.html#Examples

    double et;
    utc2et_c(date.c_str(), &et);
    check_spice_errors();

    return et;
}

std::string spyce_et_to_utc(double et, std::string format) {
    //acceptable date formats:
    //https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/cspice/utc2et_c.html#Examples

    char date_out[DATE_STR_MAX];
    et2utc_c(et, format.c_str(), 0, DATE_STR_MAX, date_out);
    check_spice_errors();

    return std::string(date_out);
}

//File Operations
namespace py = boost::python;
py::list spyce_get_objects(std::string file) {
    //NOTE: this cell is static per the macro definition
    SPICEINT_CELL(id_list, SPYCE_OBJECTS_MAX);

    //have to reset the cell so data doesn't persist per call
    scard_c(0, &id_list);
    check_spice_errors();

    py::list ret_obj;
    spkobj_c(file.c_str(), &id_list);
    check_spice_errors();

    int limit = card_c(&id_list);
    check_spice_errors();

    for(int i = 0; i < limit; i++) {
        ret_obj.append(SPICE_CELL_ELEM_I(&id_list, i));
    }

    return ret_obj;
}

namespace py = boost::python;
py::list spyce_get_coverage_windows(std::string file, int obj_id) {
    //NOTE: this cell is static per the macro definition
    SPICEDOUBLE_CELL(cover, SPYCE_OBJECTS_MAX);

    //have to reset the cell so data doesn't persist per call
    scard_c(0, &cover);
    check_spice_errors();

    py::list ret_obj;
    spkcov_c(file.c_str(), obj_id, &cover); //load coverage data of `obj` id
    check_spice_errors();

    int limit = card_c(&cover) / 2;
    check_spice_errors();

    double beg, end;
    for(int i = 0; i < limit; i++) {
        wnfetd_c(&cover, i, &beg, &end);
        check_spice_errors();

        ret_obj.append(py::make_tuple(beg,end));
    }

    return ret_obj;
}

//Kernel functions
void spyce_add_kernel(std::string s) {
    furnsh_c(s.c_str());
    check_spice_errors();
}

void spyce_remove_kernel(std::string s) {
    unload_c(s.c_str());
    check_spice_errors();
}
Frame spyce_get_frame_data(int target_id, int observer_id, double e_time) {
    SpiceDouble frame[6] = {0};
    SpiceDouble lt;

    spkez_c(
        target_id,   // target
        e_time,      // epoch time
        "J2000",     // reference frame, TODO: J2000 vs ECLIPJ2000 frame?
        "NONE",      // Aberration correction setting.
        observer_id, // observer reference
        frame,       // output frame
        &lt);        // output light time
    check_spice_errors();

    return Frame(frame);
}