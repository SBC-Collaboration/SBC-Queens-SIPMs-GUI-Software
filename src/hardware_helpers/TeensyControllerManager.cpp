#include "sbcqueens-gui/hardware_helpers/TeensyControllerManager.hpp"

// C++ STD includes
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <ratio>
#include <optional>
#include <sstream>
#include <cmath>

// C++ 3rd party includes
#include <spdlog/spdlog.h>

// my includes
#include "sbcqueens-gui/timing_events.hpp"

namespace SBCQueens {

// I have no reason to use it in the future. I am adding
// it just in case
void to_json(json& j, const Peltiers& p) {
    j = json{{"PELTIER1I", p.PID.Current}};
}

void from_json(const json& j, Peltiers& p) {
    p.time = get_current_time_epoch() / 1000.0;
    j.at("PELTIER1I").get_to(p.PID.Current);
}

void to_json(json&, const RTDs&) {
}


void from_json(const json& j, RTDs& p) {
    p.time = get_current_time_epoch() / 1000.0;
    j.at("RTDT").get_to(p.RTDS);
}

void to_json(json&, const RawRTDs&) {
}

void from_json(const json& j, RawRTDs& p) {
    p.time = get_current_time_epoch() / 1000.0;
    j.at("RTDR").get_to(p.RTDREGS);

    // for my setup is 350
    // const double kRESREFERENCE = 350.0;
    // for the boards we made, R = 120
    const double kRESREFERENCE = 120.0;

    p.Resistances = std::vector<double>(p.RTDREGS.size());
    p.Temps = std::vector<double>(p.RTDREGS.size());
    for (std::size_t i = 0; i < p.RTDREGS.size(); i++) {
        p.Resistances[i] = p.RTDREGS[i] /  32768.0;
        p.Resistances[i] *= kRESREFERENCE;

        p.Temps[i] = register_to_T90(p.Resistances[i]);
    }
}

void to_json(json& j, const Pressures& p) {
    j = json{{"VACUUMP", p.Vacuum.Pressure}};
}

void to_json(json& j, const TeensySystemPars& p) {
    j = json{{"NUM_RTD_BOARDS", p.NumRtdBoards},
    {"NUM_RTDS_PER_BOARD", p.NumRtdsPerBoard},
    {"RTD_ONLY_MODE", p.InRTDOnlyMode}};
}

void from_json(const json& j, TeensySystemPars& p) {
    j.at("NUM_RTD_BOARDS").get_to(p.NumRtdBoards);
    j.at("NUM_RTDS_PER_BOARD").get_to(p.NumRtdsPerBoard);
    j.at("RTD_ONLY_MODE").get_to(p.InRTDOnlyMode);
}

void from_json(const json& j, Pressures& p) {
    auto press = 0.0;

    p.time = get_current_time_epoch() / 1000.0;
    j.at("VACUUMP").get_to(press);
    press *= (3.32/exp2(12)) * (1.0/135);  // to current
    press = 14.6959 - (14.7/16e-3)*(press - 4e-3);  // to psi
    press /= 14.5038;  // to bar
    p.Vacuum.Pressure = press;

    // j.at("NTWOP").get_to(press);
    // press *= (3.32/exp2(12)) * (1.0/135); // to current
    // press = 14.6959 + (30/16e-3)*(press - 4e-3); // to psi
    // press /= 14.5038; //to bar
    // p.N2Line.Pressure = press;
}
// I have no reason to use it in the future. I am adding
// it just in case
void to_json(json& j, const BMEs& p) {
    j = json{{"BME1T", p.LocalBME.Temperature},
            {"BME1P", p.LocalBME.Pressure},
            {"BME1H", p.LocalBME.Humidity}};
}

void from_json(const json& j, BMEs& p) {
    // ms
    p.time = get_current_time_epoch() / 1000.0;

    int32_t temp = 0.0;
    j.at("BME1T").get_to(temp);
    double t_fine = BME_calculate_t_fine<BME_TYPE::LOCAL>(temp);
    p.LocalBME.Temperature = t_fine_to_temp(t_fine);

    j.at("BME1P").get_to(temp);
    p.LocalBME.Pressure
        = BME280_compensate_P_double<BME_TYPE::LOCAL>(temp, t_fine);

    j.at("BME1H").get_to(temp);
    p.LocalBME.Humidity
        = BME280_compensate_H_double<BME_TYPE::LOCAL>(temp, t_fine);


    // j.at("BME2T").get_to(temp);
    // t_fine = BME_calculate_t_fine<BME_TYPE::BOX>(temp);
    // p.BoxBME.Temperature = t_fine_to_temp(t_fine);

    // j.at("BME2P").get_to(temp);
    // p.BoxBME.Pressure
    //  = BME280_compensate_P_double<BME_TYPE::BOX>(temp, t_fine);

    // j.at("BME2H").get_to(temp);
    // p.BoxBME.Humidity
    //  = BME280_compensate_H_double<BME_TYPE::BOX>(temp, t_fine);
}

inline double t_fine_to_temp(const int32_t& t_fine) {
    return t_fine / 5120.0;
}

template<BME_TYPE T>
int32_t BME_calculate_t_fine(const int32_t& adc_T) {
    const BME_COMPENSATION_REGISTERS comp = bme_comp_map.at(T);

    auto adc_T_d = static_cast<double>(adc_T);
    auto dig_T1_d = static_cast<double>(comp.dig_T1);
    auto dig_T2_d = static_cast<double>(comp.dig_T2);
    auto dig_T3_d = static_cast<double>(comp.dig_T3);

    double var1 = (adc_T_d / 16384.0 - dig_T1_d/1024.0) * (dig_T2_d);

    double var2 = (((adc_T_d)/131072.0 - (dig_T1_d)/8192.0) *
    ((adc_T_d)/131072.0 - (dig_T1_d)/8192.0)) * (dig_T3_d);

    return static_cast<int32_t>(var1 + var2);
}

template<BME_TYPE T>
double BME280_compensate_P_double(const int32_t& adc_P, const int32_t& t_fine) {
    const BME_COMPENSATION_REGISTERS comp = bme_comp_map.at(T);

    double var1, var2, p;
    var1 = (static_cast<double>(t_fine)/2.0) - 64000.0;
    var2 = var1 * var1 * (static_cast<double>(comp.dig_P6)) / 32768.0;
    var2 = var2 + var1 * (static_cast<double>(comp.dig_P5)) * 2.0;
    var2 = (var2/4.0)+((static_cast<double>(comp.dig_P4)) * 65536.0);
    var1 = ((static_cast<double>(comp.dig_P3)) * var1 * var1 / 524288.0
        + (static_cast<double>(comp.dig_P2)) * var1) / 524288.0;
    var1 = (1.0 + var1 / 32768.0)*(static_cast<double>(comp.dig_P1));
    if (var1 == 0.0) {
        return 0.0;  // avoid exception caused by division by zero
    }
    p = 1048576.0 - static_cast<double>(adc_P);
    p = (p - (var2 / 4096.0)) * 6250.0 / var1;
    var1 = (static_cast<double>(comp.dig_P9)) * p * p / 2147483648.0;
    var2 = p * (static_cast<double>(comp.dig_P8)) / 32768.0;
    p = p + (var1 + var2 + (static_cast<double>(comp.dig_P7))) / 16.0;
    return p;
}

template<BME_TYPE T>
double BME280_compensate_H_double(const int32_t& adc_H, const int32_t& t_fine) {
    const BME_COMPENSATION_REGISTERS comp = bme_comp_map.at(T);

    double var_H;
    var_H = ((static_cast<double>(t_fine)) - 76800.0);
    var_H = (adc_H - ((static_cast<double>(comp.dig_H4)) * 64.0
        + (static_cast<double>(comp.dig_H5)) / 16384.0 * var_H)) *
    ((static_cast<double>(comp.dig_H2)) / 65536.0 *
        (1.0 + (static_cast<double>(comp.dig_H6)) /
    67108864.0 * var_H *
    (1.0 + (static_cast<double>(comp.dig_H3)) / 67108864.0 * var_H)));

    var_H = var_H * (1.0 -
        (static_cast<double>(comp.dig_H1)) * var_H / 524288.0);
    if (var_H > 100.0) {
        var_H = 100.0;
    }
    else if (var_H < 0.0) {
        var_H = 0.0;
    }

    return var_H;
}

double __calibrate_curve(const size_t& i, const uint16_t& count) {
    const double cal_pars[3][7] = {{5.58740460e-24, -6.28859314e-19,
    2.89813596e-14, -7.11010600e-10, 1.00457336e-05, -8.17906779e-02,
    3.37583449e+02}, {8.40079885e-24, -9.57155346e-19,  4.40061387e-14,
    -1.05319202e-09, 1.40972855e-05, -1.05041807e-01,  3.86061981e+02},
    {2.47242828e-23, -2.55552972e-18,  1.05335428e-13, -2.23119506e-09,
    2.59211509e-05, -1.62308481e-01,  4.83620434e+02}};

    arma::vec::fixed<7> curret_par = cal_pars[i];
    arma::vec::fixed<1> x = {static_cast<double>(count)};

    arma::vec out = arma::polyval(curret_par, x);

    return out(0);
}

double __res_to_temperature(const double& res) {
    const double c_RTD_NOMINAL = 100.0;
    const double c_RTD_A = 3.9083e-3;
    const double c_RTD_B = -5.775e-7;

    const double c_Z1 = -c_RTD_A;
    const double c_Z2 = c_RTD_A * c_RTD_A - (4 * c_RTD_B);
    const double c_Z3 = (4 * c_RTD_B) / c_RTD_NOMINAL;
    const double c_Z4 = 2 * c_RTD_B;

    double temp, Rtf;
    Rtf = res;

    temp = c_Z2 + (c_Z3 * Rtf);
    temp = (sqrt(temp) + c_Z1) / c_Z4;

    if (temp >= 0.0){
      return temp;
    }

    // ugh.
    Rtf /= c_RTD_NOMINAL;
    Rtf *= 100;  // normalize to 100 ohm

    double rpoly = Rtf;

    temp = -242.02;
    temp += 2.2228 * rpoly;
    rpoly *= Rtf;  // square
    temp += 2.5859e-3 * rpoly;
    rpoly *= Rtf;  // ^3
    temp -= 4.8260e-6 * rpoly;
    rpoly *= Rtf;  // ^4
    temp -= 2.8183e-8 * rpoly;
    rpoly *= Rtf;  // ^5
    temp += 1.5243e-10 * rpoly;

    return temp;
}

double register_to_T90(const double& Rtf) {
    // Obtained from
    // https://us.flukecal.com/pt100-calculator
    // With T = 273.16K
    const double kRTDTRIPLEPOINT = 100.004;
    const double W = Rtf / kRTDTRIPLEPOINT;

    // These constants obtained from
    // https://www.bipm.org/en/committees/cc/cct/guides-to-thermometry
    // Part 5, Pg 8
    const double D[] = {439.932854, 472.418020, 37.684494, 7.472018,
                      2.920828, 0.005184, -0.963864, -0.188732,
                      0.191203, 0.049025};

    double x = (W - 2.64) / 1.64;
    // This is the polynomial evaluation done in a more efficient way.
    // See https://en.wikipedia.org/wiki/Horner%27s_method
    // for more details
    double out = D[9];
    for (int i = 8; i >= 0; i--) {
      out = x*out + D[i];
    }

    // This equation is valid for 0.01degC or below
    if (out < 0.01) {
      // These constants obtained from
      // https://www.bipm.org/en/committees/cc/cct/guides-to-thermometry
      // Part 5, Pg 8
      const double B[] = {0.183324722, 0.240975303, 0.209108711, 0.190439972,
                        0.142648498, 0.077993465, 0.012475611, -0.032267127,
                        -0.075291522, -0.056470670, 0.076201285, 0.123893204,
                        -0.029201193, -0.091173542, 0.001317696, 0.026025526};

      x = (pow(W, 1.0/6.0) - 0.65) / 0.35;
      out = B[15];
      for (int i = 14; i >= 0; i--) {
        out = x*out + B[i];
      }

      return 273.16*out;
    } else {
      return out + 273.15;
    }
  }

}  // namespace SBCQueens
