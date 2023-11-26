#INCLUDE <SPEECH-RECOGNITION-GROW-TECH_INFERENCING.H>
#INCLUDE "DRIVER/I2S.H"
#DEFINE BLYNK_PRINT SERIAL
#INCLUDE <WIFI.H>
#INCLUDE <WIFICLIENT.H>
#INCLUDE <BLYNKSIMPLEESP32.H>
//ESP AND I2S DRIVER SETUP
CONST I2S_PORT_T I2S_PORT = I2S_NUM_0;
ESP_ERR_T ERR;
#DEFINE I2S_SAMPLE_RATE (16000)
INT LED = 13; 
INT RLED = 12; 
INT LASTREC = 0;
// YOU SHOULD GET AUTH TOKEN IN THE BLYNK APP.
// GO TO THE PROJECT SETTINGS (NUT ICON).
CHAR AUTH[] = "7GZRXXLSWPREROG03HBQE4QVGNBKFS2S";

// YOUR WIFI CREDENTIALS.
// SET PASSWORD TO "" FOR OPEN NETWORKS.
CHAR SSID[] = "IPHONE 11";
CHAR PASS[] = "SHETTY@2023";
WIDGETLED RED(V1);347433
WIDGETLED GREEN(V2);
// THE I2S CONFIG AS PER THE EXAMPLE
  CONST I2S_CONFIG_T I2S_CONFIG = {
      .MODE = I2S_MODE_T(I2S_MODE_MASTER | I2S_MODE_RX), // RECEIVE, NOT TRANSFER
      .SAMPLE_RATE = I2S_SAMPLE_RATE,                         // 16KHZ
      .BITS_PER_SAMPLE = I2S_BITS_PER_SAMPLE_16BIT, // COULD ONLY GET IT TO WORK WITH 32BITS
      .CHANNEL_FORMAT = I2S_CHANNEL_FMT_RIGHT_LEFT, // USE RIGHT CHANNEL
      .COMMUNICATION_FORMAT = I2S_COMM_FORMAT_T(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
      .INTR_ALLOC_FLAGS = ESP_INTR_FLAG_LEVEL1,     // INTERRUPT LEVEL 1
      .DMA_BUF_COUNT = 4,                           // NUMBER OF BUFFERS
      .DMA_BUF_LEN = 8                              // 8 SAMPLES PER BUFFER (MINIMUM)
  };

// THE PIN CONFIG AS PER THE SETUP
  CONST I2S_PIN_CONFIG_T PIN_CONFIG = {
      .BCK_IO_NUM = 26,   // SERIAL CLOCK (SCK)
      .WS_IO_NUM = 14,    // WORD SELECT (WS)
      .DATA_OUT_NUM = I2S_PIN_NO_CHANGE, // NOT USED (ONLY FOR SPEAKERS)
      .DATA_IN_NUM = 32   // SERIAL DATA (SD)
  };

INT16_T SAMPLEBUFFER[16000]; 
INT16_T FEATURES[16000];  

INT I2SREAD(){
  SIZE_T BYTESREAD;
 
  DIGITALWRITE(RLED, HIGH); 
  RED.ON();
  SERIAL.PRINTLN(" *** RECORDING START ***"); 

  INT COUNT = 0; 
  LASTREC = MILLIS(); 
  WHILE(1){
    I2S_READ(I2S_PORT, (VOID*) SAMPLEBUFFER, 4, &BYTESREAD, PORTMAX_DELAY);
    IF(*SAMPLEBUFFER < (-40) || MILLIS() - LASTREC >= 6000){
      FOR(INT I = 0;I < 16000; I++){
        I2S_READ(I2S_PORT, (VOID*) SAMPLEBUFFER, 4, &BYTESREAD, PORTMAX_DELAY);
        FEATURES[I] = SAMPLEBUFFER[0]; 
       }
       BREAK; 
    }
  }
  SERIAL.PRINTLN(" *** RECORDING ENDED *** "); 
  DIGITALWRITE(RLED, LOW); 
  RED.LOW();
  RETURN BYTESREAD; 
} 

INT RAW_GET_DATA(SIZE_T OFFSET, SIZE_T LENGTH, FLOAT *OUT_PTR) {
    RETURN NUMPY::INT16_TO_FLOAT(FEATURES + OFFSET, OUT_PTR, LENGTH);
}
WIDGETTERMINAL TERMINAL(V10);

VOID SETUP()
{
    // PUT YOUR SETUP CODE HERE, TO RUN ONCE:
    SERIAL.BEGIN(115200);
    BLYNK.BEGIN(AUTH, SSID, PASS,IPADDRESS(117,236,190,213), 8080);
    SERIAL.PRINTLN("EDGE IMPULSE INFERENCING DEMO");

    ERR = I2S_DRIVER_INSTALL(I2S_PORT, &I2S_CONFIG, 0, NULL);
    I2S_SET_PIN(I2S_PORT, &PIN_CONFIG);

    PINMODE(33, OUTPUT); 
    PINMODE(27, INPUT); 
    PINMODE(25, OUTPUT); 
    PINMODE(LED, OUTPUT); 
    PINMODE(RLED, OUTPUT); 

}

VOID LOOP()
{
    EI_PRINTF("EDGE IMPULSE STANDALONE INFERENCING (ARDUINO)\N");

    //RECORD AUDIO
    INT BYTESREAD = I2SREAD();
    //FOR(INT I = 0; I < 50; I++)  
      //SERIAL.PRINT(FEATURES[I]); 

/*
    IF (SIZEOF(FEATURES) / SIZEOF(FLOAT) != EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
        EI_PRINTF("THE SIZE OF YOUR 'FEATURES' ARRAY IS NOT CORRECT. EXPECTED %LU ITEMS, BUT HAD %LU\N",
            EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, SIZEOF(FEATURES) / SIZEOF(FLOAT));
        DELAY(1000);
        RETURN;
    }
*/

    EI_IMPULSE_RESULT_T RESULT = { 0 };

    // THE FEATURES ARE STORED INTO FLASH, AND WE DON'T WANT TO LOAD EVERYTHING INTO RAM
    SIGNAL_T SIGNAL;
    SIGNAL.TOTAL_LENGTH = 16000; 
    SIGNAL.GET_DATA = &RAW_GET_DATA; 

    // INVOKE THE IMPULSE
    EI_IMPULSE_ERROR RES = RUN_CLASSIFIER(&SIGNAL, &RESULT, FALSE /* DEBUG */);
    EI_PRINTF("RUN_CLASSIFIER RETURNED: %D\N", RES);

    IF (RES != 0) RETURN;

    // PRINT THE PREDICTIONS
    EI_PRINTF("PREDICTIONS ");
    EI_PRINTF("(DSP: %D MS., CLASSIFICATION: %D MS., ANOMALY: %D MS.)",
        RESULT.TIMING.DSP, RESULT.TIMING.CLASSIFICATION, RESULT.TIMING.ANOMALY);
    EI_PRINTF(": \N");
    EI_PRINTF("[");
    FOR (SIZE_T IX = 0; IX < EI_CLASSIFIER_LABEL_COUNT; IX++) {
        EI_PRINTF("%.5F", RESULT.CLASSIFICATION[IX].VALUE);
#IF EI_CLASSIFIER_HAS_ANOMALY == 1
        EI_PRINTF(", ");
#ELSE
        IF (IX != EI_CLASSIFIER_LABEL_COUNT - 1) {
            EI_PRINTF(", ");
        }
#ENDIF
    }
#IF EI_CLASSIFIER_HAS_ANOMALY == 1
    EI_PRINTF("%.3F", RESULT.ANOMALY);
#ENDIF
    EI_PRINTF("]\N");

    // HUMAN-READABLE PREDICTIONS
    FOR (SIZE_T IX = 0; IX < EI_CLASSIFIER_LABEL_COUNT; IX++) {
        EI_PRINTF("    %S: %.5F\N", RESULT.CLASSIFICATION[IX].LABEL, RESULT.CLASSIFICATION[IX].VALUE);
    }
#IF EI_CLASSIFIER_HAS_ANOMALY == 1
    EI_PRINTF("    ANOMALY SCORE: %.3F\N", RESULT.ANOMALY);
    TERMINAL.PRINT("ANOMALY SCORE: %.3F\N", RESULT.ANOMALY)
#ENDIF

  SERIAL.PRINT("GROW-TECH: "); 
  // TERMINAL.PRINT("GROW-TECH: "); 
  SERIAL.PRINTLN(RESULT.CLASSIFICATION[2].VALUE); 
  // TERMINAL.PRINTLN(RESULT.CLASSIFICATION[2].VALUE); 
  IF(RESULT.CLASSIFICATION[2].VALUE > 0.8){
    TERMINAL.PRINTLN("KEYWORD DETECTED");
    DIGITALWRITE(LED, HIGH);
    GREEN.ON(); 
    DELAY(1000); 
    GREEN.ON();
    DIGITALWRITE(LED, LOW); 
    DELAY(1000); 
  }
  ELSE
  {
    TERMINAL.PRINTLN("KEYWORD NOT DETECTED");
  }

}

/**
 * @BRIEF      PRINTF FUNCTION USES VSNPRINTF AND OUTPUT USING ARDUINO SERIAL
 *
 * @PARAM[IN]  FORMAT     VARIABLE ARGUMENT LIST
 */

VOID EI_PRINTF(CONST CHAR *FORMAT, ...) {
    STATIC CHAR PRINT_BUF[1024] = { 0 };
    VA_LIST ARGS;
    VA_START(ARGS, FORMAT);
    INT R = VSNPRINTF(PRINT_BUF, SIZEOF(PRINT_BUF), FORMAT, ARGS);
    VA_END(ARGS);
    IF (R > 0) {
        SERIAL.WRITE(PRINT_BUF);
    }
    BLYNK.RUN();
}
