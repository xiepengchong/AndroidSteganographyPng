#include <jni.h>
#include <string>
#include <android/asset_manager_jni.h>
#include "log.h"
#include <errno.h>
#include <android/asset_manager.h>


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
FILE* android_fopen(const char* fname, const char* mode) {
    if(mode[0] == 'w') return NULL;
    AAsset* asset = AAssetManager_open(mgr, fname, 0);
    if(!asset) return NULL;

    return funopen(asset, android_read, android_write, android_seek, android_close);
}



/**
 * 写入文件到sdcard中
 * @param output
 * @return
 */
FILE* android_fopen_sd(char* output){
    FILE* file = fopen(output,"w+");
    return file;
}

/**
 * 读取sdcard中的文件
 * @param output
 * @return
 */
FILE* android_fopen_rd(char* output){
    FILE* file = fopen(output,"rb");
    return file;
}

unsigned int GetCrc32(unsigned char* InStr,unsigned int len){
    unsigned int Crc32Table[256];
    unsigned int i,j;
    unsigned int Crc;
    for (i = 0; i < 256; i++){
        Crc = i;
        for (j = 0; j < 8; j++){
            if (Crc & 1)
                Crc = (Crc >> 1) ^ 0xEDB88320;
            else
                Crc >>= 1;
        }
        Crc32Table[i] = Crc;
    }

    Crc=0xffffffff;
    for(unsigned int m=0; m<len; m++){
        Crc = (Crc >> 8) ^ Crc32Table[(Crc & 0xFF) ^ InStr[m]];
    }

    Crc ^= 0xFFFFFFFF;
    return Crc;
}

void write_text(FILE *fp,char* password)
{
    unsigned char *buf;
    int len;
    len=strlen(password);
    buf=new unsigned char[len+12];
    buf[0]=len>>24&0xff;
    buf[1]=len>>16&0xff;
    buf[2]=len>>8&0xff;
    buf[3]=len&0xff;
    buf[4]='t';
    buf[5]='E';
    buf[6]='X';
    buf[7]='t';
    for(int j=0;j<len;j++)
        buf[j+8]=password[j];
    buf[len+8]=0XFA;
    buf[len+9]=0XC4;
    buf[len+10]=0X08;
    buf[len+11]=0X76;
    fwrite(buf,len+12,1,fp);
}


char* getPassword(char *input) {

    FILE *fpnew;
    if ((fpnew = android_fopen(input,"rb")) == NULL) {
        fclose(fpnew);
        return "";
    }

    fseek (fpnew , 0 , SEEK_END);
    int lSize = ftell (fpnew);

    rewind (fpnew);

    char * buffer = (char*) malloc (sizeof(char)*lSize);
    if (buffer == NULL)
    {
        LOGD ("Memory error",stderr);
        exit (2);
    }

    size_t result = fread (buffer, 1,lSize,fpnew);
    buffer[lSize]='\0';
    for(int i=0;i<result;i++){
        if(buffer[i] == NULL){
            buffer[i] = '_';
        }
    }
    if (result != lSize) {
        LOGD("Reading error", stderr);
        exit(3);
    }
    std::string hello = buffer;
    int start = hello.find("tEXt");
    int end = hello.find("IEND");
    char t[16];
    int n = end-start-12;
    strncpy(t, (char *) buffer + start+4, n);
    t[n]='\0';
    return t;
}

int write_password_text(char* password,char* filename,char* output){
    FILE *fp,*fpnew;
    unsigned char *buf=NULL;
    unsigned int len=0;
    unsigned int ChunkLen=0;
    unsigned int ChunkCRC32=0;
    unsigned int ChunkOffset=0;
    unsigned int crc32=0;
    unsigned int i=0,j=0;
    unsigned char Signature[8]={0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a};
    unsigned char IEND[12]={0x00,0x00,0x00,0x00,0x49,0x45,0x4e,0x44,0xae,0x42,0x60,0x82};

    if((fp=android_fopen(filename,"rb"))==NULL){
        LOGD("fopen failed file=%s",filename);
        return 0;
    }

    if((fpnew=android_fopen_sd(output))==NULL){
        LOGD("fopen failed file=%s",output);
        return 0;
    }
    fseek(fp,0,SEEK_END);
    len=ftell(fp);
    buf=new unsigned char[len];
    fseek(fp,0,SEEK_SET);
    fread(buf,len,1,fp);
    fseek(fp,8,SEEK_SET);
    ChunkOffset=8;
    i=0;
    fwrite(Signature,8,1,fpnew);
    while(1)
    {
        i++;
        j=0;
        memset(buf,0,len);
        fread(buf,4,1,fp);
        fwrite(buf,4,1,fpnew);
        ChunkLen=(buf[0]<<24)|(buf[1]<<16)|(buf[2]<<8)|buf[3];
        fread(buf,4+ChunkLen,1,fp);
        if(strncmp((char *)buf,"IHDR",4)==0|strncmp((char *)buf,"PLTE",4)==0|strncmp((char *)buf,"IDAT",4)==0)
        {
            fwrite(buf,4+ChunkLen,1,fpnew);
        }
        else
        {
            fseek(fpnew,-4,SEEK_CUR);
            j=1;
        }
        crc32=GetCrc32(buf,ChunkLen+4);
        fread(buf,4,1,fp);
        ChunkCRC32=(buf[0]<<24)|(buf[1]<<16)|(buf[2]<<8)|buf[3];
        if(crc32!=ChunkCRC32)
            LOGD("[!]CRC32Check Error!\n");
        else
        {
            LOGD("Check Success!\n\n");
            if(j==0)
                fwrite(buf,4,1,fpnew);
        }
        ChunkLen=ftell(fp);
        if(ChunkLen==(len-12))
        {
            LOGD("Total Chunk:%d",i);
            break;
        }
    }
    write_text(fpnew,password);
    fwrite(IEND,12,1,fpnew);
    fclose(fp);
    fclose(fpnew);
    return 1;
}

extern "C" JNIEXPORT jint

JNICALL
Java_steganography_com_steganographydemo_MainActivity_encode(
        JNIEnv *env,
        jobject /* this */,jobject assetManger,jstring string,jstring filename,jstring outputName) {
    std::string hello = "Hello from C++";
    mgr = AAssetManager_fromJava(env, assetManger);
    const char *nativeString = env->GetStringUTFChars(string, JNI_FALSE);
    const char *nativeFileName = env->GetStringUTFChars(filename, JNI_FALSE);
    const char *nativeSOutputName = env->GetStringUTFChars(outputName, JNI_FALSE);
    int result = write_password_text((char *)nativeString,(char *)nativeFileName,(char *)nativeSOutputName);
    return result;
}
extern "C" JNIEXPORT jstring

JNICALL
Java_steganography_com_steganographydemo_MainActivity_decode(
        JNIEnv *env,
        jobject /* this */,jobject assetManger,jstring string) {
    mgr = AAssetManager_fromJava(env, assetManger);
    const char *nativeString = env->GetStringUTFChars(string, JNI_FALSE);
    char *output = getPassword((char *) nativeString);

    return env->NewStringUTF(output);
}