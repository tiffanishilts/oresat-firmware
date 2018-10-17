#include "sin_lut.h"

static const dutycycle_t sin_lut[LUT_SIZE] = 
{
  5000,5061,5123,5184,5245,5307,5368,5429,
  5490,5551,5612,5673,5734,5794,5855,5915,
  5975,6036,6096,6155,6215,6274,6334,6393,
  6451,6510,6568,6627,6684,6742,6799,6857,
  6913,6970,7026,7082,7138,7193,7248,7303,
  7357,7411,7464,7518,7571,7623,7675,7727,
  7778,7829,7879,7929,7978,8028,8076,8124,
  8172,8219,8266,8312,8358,8403,8448,8492,
  8536,8579,8621,8663,8705,8746,8786,8826,
  8865,8904,8942,8979,9016,9052,9088,9123,
  9157,9191,9224,9257,9289,9320,9350,9380,
  9410,9438,9466,9493,9520,9546,9571,9596,
  9619,9643,9665,9687,9708,9728,9748,9767,
  9785,9802,9819,9835,9850,9865,9879,9892,
  9904,9916,9926,9937,9946,9955,9962,9970,
  9976,9982,9986,9991,9994,9997,9998,10000,
  10000,10000,9998,9997,9994,9991,9986,9982,
  9976,9970,9962,9955,9946,9937,9926,9916,
  9904,9892,9879,9865,9850,9835,9819,9802,
  9785,9767,9748,9728,9708,9687,9665,9643,
  9619,9596,9571,9546,9520,9493,9466,9438,
  9410,9380,9350,9320,9289,9257,9224,9191,
  9157,9123,9088,9052,9016,8979,8942,8904,
  8865,8826,8786,8746,8705,8663,8621,8579,
  8536,8492,8448,8403,8358,8312,8266,8219,
  8172,8124,8076,8028,7978,7929,7879,7829,
  7778,7727,7675,7623,7571,7518,7464,7411,
  7357,7303,7248,7193,7138,7082,7026,6970,
  6913,6857,6799,6742,6684,6627,6568,6510,
  6451,6393,6334,6274,6215,6155,6096,6036,
  5975,5915,5855,5794,5734,5673,5612,5551,
  5490,5429,5368,5307,5245,5184,5123,5061,
  5000,4939,4877,4816,4755,4693,4632,4571,
  4510,4449,4388,4327,4266,4206,4145,4085,
  4025,3964,3904,3845,3785,3726,3666,3607,
  3549,3490,3432,3373,3316,3258,3201,3143,
  3087,3030,2974,2918,2862,2807,2752,2697,
  2643,2589,2536,2482,2429,2377,2325,2273,
  2222,2171,2121,2071,2022,1972,1924,1876,
  1828,1781,1734,1688,1642,1597,1552,1508,
  1464,1421,1379,1337,1295,1254,1214,1174,
  1135,1096,1058,1021,984,948,912,877,
  843,809,776,743,711,680,650,620,
  590,562,534,507,480,454,429,404,
  381,357,335,313,292,272,252,233,
  215,198,181,165,150,135,121,108,
  96,84,74,63,54,45,38,30,
  24,18,14,9,6,3,2,0,
  0,0,2,3,6,9,14,18,
  24,30,38,45,54,63,74,84,
  96,108,121,135,150,165,181,198,
  215,233,252,272,292,313,335,357,
  381,404,429,454,480,507,534,562,
  590,620,650,680,711,743,776,809,
  843,877,912,948,984,1021,1058,1096,
  1135,1174,1214,1254,1295,1337,1379,1421,
  1464,1508,1552,1597,1642,1688,1734,1781,
  1828,1876,1924,1972,2022,2071,2121,2171,
  2222,2273,2325,2377,2429,2482,2536,2589,
  2643,2697,2752,2807,2862,2918,2974,3030,
  3087,3143,3201,3258,3316,3373,3432,3490,
  3549,3607,3666,3726,3785,3845,3904,3964,
  4025,4085,4145,4206,4266,4327,4388,4449,
  4510,4571,4632,4693,4755,4816,4877,4939
};

