/*
 * sine.c
 *
 *  Created on: Nov 1, 2020
 *      Author: monke
 */
#include "sine.h"
//Generate a wave here:
//https://www.daycounter.com/Calculators/Sine-Generator-Calculator.phtml

 uint32_t wave[2*FREQ_SAMP] = {
		 2048,2090,2133,2176,2219,2262,2304,2347,
		 2389,2431,2473,2515,2557,2598,2639,2680,
		 2721,2761,2801,2841,2880,2919,2958,2996,
		 3034,3071,3108,3145,3181,3216,3251,3285,
		 3319,3353,3385,3418,3449,3480,3510,3540,
		 3569,3597,3625,3652,3678,3704,3729,3753,
		 3776,3799,3821,3842,3862,3881,3900,3918,
		 3935,3951,3967,3981,3995,4008,4020,4031,
		 4041,4050,4059,4066,4073,4079,4084,4088,
		 4091,4093,4095,4095,4095,4093,4091,4088,
		 4084,4079,4073,4066,4059,4050,4041,4031,
		 4020,4008,3995,3981,3967,3951,3935,3918,
		 3900,3881,3862,3842,3821,3799,3776,3753,
		 3729,3704,3678,3652,3625,3597,3569,3540,
		 3510,3480,3449,3418,3385,3353,3319,3285,
		 3251,3216,3181,3145,3108,3071,3034,2996,
		 2958,2919,2880,2841,2801,2761,2721,2680,
		 2639,2598,2557,2515,2473,2431,2389,2347,
		 2304,2262,2219,2176,2133,2090,2048,2005,
		 1962,1919,1876,1833,1791,1748,1706,1664,
		 1622,1580,1538,1497,1456,1415,1374,1334,
		 1294,1254,1215,1176,1137,1099,1061,1024,
		 987,950,914,879,844,810,776,742,
		 710,677,646,615,585,555,526,498,
		 470,443,417,391,366,342,319,296,
		 274,253,233,214,195,177,160,144,
		 128,114,100,87,75,64,54,45,
		 36,29,22,16,11,7,4,2,
		 0,0,0,2,4,7,11,16,
		 22,29,36,45,54,64,75,87,
		 100,114,128,144,160,177,195,214,
		 233,253,274,296,319,342,366,391,
		 417,443,470,498,526,555,585,615,
		 646,677,710,742,776,810,844,879,
		 914,950,987,1024,1061,1099,1137,1176,
		 1215,1254,1294,1334,1374,1415,1456,1497,
		 1538,1580,1622,1664,1706,1748,1791,1833,
		 1876,1919,1962,2005,2048,

		 2048,2090,2133,2176,2219,2262,2304,2347,
		 2389,2431,2473,2515,2557,2598,2639,2680,
		 2721,2761,2801,2841,2880,2919,2958,2996,
		 3034,3071,3108,3145,3181,3216,3251,3285,
		 3319,3353,3385,3418,3449,3480,3510,3540,
		 3569,3597,3625,3652,3678,3704,3729,3753,
		 3776,3799,3821,3842,3862,3881,3900,3918,
		 3935,3951,3967,3981,3995,4008,4020,4031,
		 4041,4050,4059,4066,4073,4079,4084,4088,
		 4091,4093,4095,4095,4095,4093,4091,4088,
		 4084,4079,4073,4066,4059,4050,4041,4031,
		 4020,4008,3995,3981,3967,3951,3935,3918,
		 3900,3881,3862,3842,3821,3799,3776,3753,
		 3729,3704,3678,3652,3625,3597,3569,3540,
		 3510,3480,3449,3418,3385,3353,3319,3285,
		 3251,3216,3181,3145,3108,3071,3034,2996,
		 2958,2919,2880,2841,2801,2761,2721,2680,
		 2639,2598,2557,2515,2473,2431,2389,2347,
		 2304,2262,2219,2176,2133,2090,2048,2005,
		 1962,1919,1876,1833,1791,1748,1706,1664,
		 1622,1580,1538,1497,1456,1415,1374,1334,
		 1294,1254,1215,1176,1137,1099,1061,1024,
		 987,950,914,879,844,810,776,742,
		 710,677,646,615,585,555,526,498,
		 470,443,417,391,366,342,319,296,
		 274,253,233,214,195,177,160,144,
		 128,114,100,87,75,64,54,45,
		 36,29,22,16,11,7,4,2,
		 0,0,0,2,4,7,11,16,
		 22,29,36,45,54,64,75,87,
		 100,114,128,144,160,177,195,214,
		 233,253,274,296,319,342,366,391,
		 417,443,470,498,526,555,585,615,
		 646,677,710,742,776,810,844,879,
		 914,950,987,1024,1061,1099,1137,1176,
		 1215,1254,1294,1334,1374,1415,1456,1497,
		 1538,1580,1622,1664,1706,1748,1791,1833,
		 1876,1919,1962,2005,2048,
};

int asin_lut[4096];
void gen_asin(){
    for(int i = 0;i<4096;i++){
        double phase = asin((i-2048.0)/2048.0);
        asin_lut[i] = phase*40000000;
    }
}
