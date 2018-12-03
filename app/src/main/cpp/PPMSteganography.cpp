#include <stdio.h>
#include "stego.h"
#include <android/asset_manager_jni.h>
#include "log.h"
#include <errno.h>
#include <android/asset_manager.h>
#include <string>

void copy_header(FILE *, int, FILE *);
int get_message_length(char[]);
int message_fits(int, int, int);
int count_new_lines(FILE *);
void encode_length(FILE *, FILE *, int);
void encode_message(FILE *, FILE *, int, char *, int, int);


static AAssetManager* mgr;
static int android_read(void* cookie, char* buf, int size) {
    return AAsset_read((AAsset*)cookie, buf, size);
}

static int android_write(void* cookie, const char* buf, int size) {
    return EACCES; // can't provide write access to the apk
}

static fpos_t android_seek(void* cookie, fpos_t offset, int whence) {
    return AAsset_seek((AAsset*)cookie, offset, whence);
}

static int android_close(void* cookie) {
    AAsset_close((AAsset*)cookie);
    return 0;
}


/**
 * 打开asset中的文件
 * @param fname
 * @param mode
 * @return
 */
FILE* android_fopen_ppm(const char *fname, const char *mode) {
    if(mode[0] == 'w') return NULL;
    AAsset* asset = AAssetManager_open(mgr, fname, 0);
    if(!asset) return NULL;

    return funopen(asset, android_read, android_write, android_seek, android_close);
}



int get_msg_length(FILE *fp) {
    char temp = 0;
    int length = 0;
    int i;

    for(i=0; i < 8; i++) {
        temp = fgetc(fp);
        //Delay the shift by one to get the 29, otherwise I'd get 58
        if(i > 0) length <<= 1;
        length |= (temp & 1);
    }

    return length;
}

char * decode_message(int length, FILE *fp) {
    int readTo = length * 16, numRead = 0, i;
    unsigned char charBuffer = 0;
    char temp;
    char secret_message[length + 1];
    int idx = 0;
    while(numRead <= readTo) {
        temp = fgetc(fp);
        if(numRead % 16 == 0) {
            secret_message[idx] = charBuffer;
            idx++;
            charBuffer = 0;
        } else {
            charBuffer <<= 1;
        }
        charBuffer |= (temp & 1);
        numRead++;
    }//end while
    for(i = 1; i < idx; i++) {
        secret_message[i-1] = secret_message[i];
    }
    secret_message[idx-1] = '\0';

//    //Start printing from character 1 because the first char is junk
//    for(i = 1; i < idx; i++) {
//        printf("%c", secret_message[i]);
//    }
//
//    printf("\n\n");

    return secret_message;
}


std::string decode(const char* filename){
    FILE *fp;
    char t[32];
    //Print an error if no args provided
    if((fp = android_fopen_ppm(filename, "rb")) == NULL) {
        LOGD("\nError: Please provide a file to scan.\n\n");
        return "";
    }

    if(read_ppm_type(fp)) {
        skip_comments(fp);
        get_width(fp);
        get_height(fp);
        if(read_color_depth(fp)) {
            int length = get_msg_length(fp);
            LOGD("length=%d",length);

            char * result;

            result = decode_message(length, fp);

            for(int i=0;i<length;i++){
                t[i] = result[i];
            }
            t[length] = '\0';

            fclose(fp);
            LOGD("t=%s",t);
            return (std::string)t;
        } else {
            LOGD("Error: Invalid Color Depth.");
            return t;
        }
    } else {
        LOGD("Error: Wrong PPM File Format. Terminating.");
        return t;
    }
}

int encode(const char * password,const char* filename, const char * output) {

    FILE *fp;
    if((fp = android_fopen_ppm(filename, "r+")) == NULL) {
        LOGD("Could not open file %s.\nAborting\n", filename);
        return 0;
    }

    if(read_ppm_type(fp)) {
        skip_comments(fp);

        char *myMessage = const_cast<char *>(password);
        int message_length = get_message_length(myMessage);
        int w = get_width(fp);
        int h = get_height(fp);

        if(message_fits(message_length, w, h)) {
            if(read_color_depth(fp)) {
                FILE *fp_t = fopen(output,"w");
                int i = count_new_lines(fp);
                copy_header(fp, i, fp_t);
                encode_length(fp, fp_t, (message_length - 8) / 16);
                encode_message(fp, fp_t, (message_length - 8), myMessage, w, h);
                LOGD("Encoding Process Complete. Take a look at out.ppm\n");

                fclose(fp);
                fclose(fp_t);
            } else {
                LOGD("\nError: Image color depth invalid. Must be 255.\n");
                return 0;
            }
        } else {
            LOGD("\nError: Image file is not large enough to hold the message.\nAborting.\n");
            return 0;
        }
    } else {
        LOGD("Error: Wrong file format.\nAborting\n");
        return 0;
    }

    return 1;
}

void copy_header(FILE *fp1, int numLines, FILE *fp2) {
    int i;
    char temp;

    rewind(fp1); //Goes back to the beginning of the file

    for(i = 0; i < numLines; i++) {
        while((temp = fgetc(fp1)) != EOF && temp != '\n') {
            fputc(temp, fp2);
        }
        fputc('\n', fp2);
    }

    return;
}
int get_message_length(char my_msg[]) {
    int i = 0;
    while(my_msg[i] != '\0') {
        i++;
    }
    return i * 16 + 8;
}
int message_fits(int length, int w, int h) {
    return length < w * h * 3;
}
int count_new_lines(FILE *fp) {
    char temp; int count = 0;

    rewind(fp);

    while((temp = fgetc(fp)) != EOF)
        if(temp == '\n')
            count++;

    return count;
}

void encode_length(FILE *in, FILE *out, int length) {
    char temp;
    int i, l;
    for(i = 0; i < 8; i++) {
        temp = fgetc(in);
        l = length;
        l >>= 7 - i;
        if((l & 1) == 1) {
            if((temp & 1) == 0) {
                temp++;
            }
        } else {
            if((temp & 1) == 1) {
                temp--;
            }
        }
        fputc(temp, out);
    }

    return;
}

void encode_message(FILE *in, FILE *out, int num_to_encode, char* my_message, int w, int h) {
    int encoded = 0;
    unsigned char temp;
    int idx = 0, num_coppied = 0;
    char current;

    int fileSize = (w * h * 3) - 8; //Number of bytes after first 8
    int i;

    for(i = 0; i < fileSize; i++) {
        temp = fgetc(in);
        current = my_message[idx];

        current >>= 15 - num_coppied;
        num_coppied++;

        if(encoded <= num_to_encode) {
            encoded++;
            if((current & 1) == 1) {
                if((temp & 1) == 0) {
                    temp++;
                }
            } else {
                if((temp & 1) == 1) {
                    temp--;
                }
            }
            if(num_coppied == 16) {
                idx++;
                num_coppied = 0;
            }
        }

        fputc(temp, out);
    }

    return;
}


extern "C" JNIEXPORT jint

JNICALL
Java_steganography_com_steganographydemo_MainActivity_encodePMM(
        JNIEnv *env,
        jobject /* this */,jobject assetManger,jstring string,jstring filename,jstring outputName) {
    mgr = AAssetManager_fromJava(env, assetManger);
    const char *nativeString = env->GetStringUTFChars(string, JNI_FALSE);
    const char *nativeFileName = env->GetStringUTFChars(filename, JNI_FALSE);
    const char *nativeSOutputName = env->GetStringUTFChars(outputName, JNI_FALSE);
    int result = encode((char *)nativeString,(char *)nativeFileName,(char *)nativeSOutputName);
    return result;
}
extern "C" JNIEXPORT jstring

JNICALL
Java_steganography_com_steganographydemo_MainActivity_decodePMM(
        JNIEnv *env,
        jobject /* this */,jobject assetManger,jstring string) {
    mgr = AAssetManager_fromJava(env, assetManger);
    const char *nativeString = env->GetStringUTFChars(string, JNI_FALSE);
    std::string output = decode((char *) nativeString);
    return env->NewStringUTF(output.c_str());
}
