
cd ../../

pwd

examples=($(find examples/* -maxdepth 1 -type d -printf "%f\n" | grep -E "^(SY6970|BQ25896)"))
# examples=($(find examples/* -maxdepth 1 -type d -printf "%f\n" | awk '/^ESP_IDF/ {next} ;/^main/ {next} ;!/Linux$/'))
# echo "Filtered directories:"
# for example in "${examples[@]}"; do
#     echo "$example"
# done
# exit

envs=(
    # "esp32s3"
    "nrf52840"
    )

pio run -t clean

for env in ${envs[@]}
do
    for value in ${examples[@]}
    do
        if [ -f "$value/.skip."$env ];then
            echo "Skip" $value
            continue
        fi

        export PLATFORMIO_SRC_DIR="examples/$value"
        export PLATFORMIO_BUILD_FLAGS="-D XPOWERS_NO_ERROR"
        echo "PLATFORMIO_SRC_DIR=$PLATFORMIO_SRC_DIR , ENV: $env" 
        pio run -e $env 
        if [ $? -ne 0 ]; then
            echo "Build env: $env $PLATFORMIO_SRC_DIR Failed!"
            exit -1
        else
            echo "Build env: $env $PLATFORMIO_SRC_DIR Successed!"
        fi
    done
done

echo "Build directories:"
for example in "${examples[@]}"; do
    echo "$example"
done









