package steganography.com.steganographydemo

import android.content.res.AssetManager
import android.support.v7.app.AppCompatActivity
import android.os.Bundle
import android.os.Environment
import android.util.Log
import android.view.View
import android.widget.Toast
import kotlinx.android.synthetic.main.activity_main.*

class MainActivity : AppCompatActivity(), View.OnClickListener {
    override fun onClick(v: View?) {
        if(v == encode){
            var outputFile = Environment.getExternalStorageDirectory().absolutePath+"/output.png"
            var result = encode(assets,"12345678","input.png",outputFile)
            if(result == 1){
                Toast.makeText(this,"加密成功，请查看文件:"+outputFile,Toast.LENGTH_LONG).show()
            } else {
                Toast.makeText(this,"加密失败",Toast.LENGTH_LONG).show()
            }
        } else if(v == decode){
            var password = decode(assets,"encode.png");
            password_value.text = "从图片解析出来的密码是:"+password;
        } else if(v == encodePMM){
            var outputFile = Environment.getExternalStorageDirectory().absolutePath+"/output.png"
            var result = encodePMM(assets,"12345678","hackny.ppm",outputFile)
            if(result == 1){
                Toast.makeText(this,"加密成功，请查看文件:"+outputFile,Toast.LENGTH_LONG).show()
            } else {
                Toast.makeText(this,"加密失败",Toast.LENGTH_LONG).show()
            }
        } else if(v == decodePMM){
            var password = decodePMM(assets,"output.png")
            password_value.text = "从图片解析出来的密码是:"+password
        }

    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        encode.setOnClickListener(this)
        decode.setOnClickListener(this)
        decodePMM.setOnClickListener(this)
        encodePMM.setOnClickListener(this)
    }

    /**
     * @param asset assetmanager
     * @param save the password to sdcard image
     * @param filename the filename of asset folder
     * @param output the file Path to generate file  eg: /sdcard/output.png
     */
    external fun encode(asset:AssetManager, password:String,filename:String,output:String): Int

    /**
     * @param asset assetmanager
     * @param filename filename of asset folder
     */
    external fun decode(asset:AssetManager,filename:String): String


    external fun encodePMM(asset:AssetManager, password:String,filename:String,output:String): Int

    external fun decodePMM(asset:AssetManager,filename:String): String

    companion object {

        // Used to load the 'native-lib' library on application startup.
        init {
            System.loadLibrary("native-lib")
        }
    }
}
