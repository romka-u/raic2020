zip submit.zip -r model/* *.h *.{c,h}pp CMakeLists.txt 2>&1 1>/dev/null

wc -l *.h *.cpp

for i in $(seq -f "%04g" 1 1991); do
    fs="submits/submit${i}.zip"
    if [ ! -f "$fs" ]; then
        echo "Copy submit.zip to $fs and binary"
        cp submit.zip $fs
        cp build/aicup2020 build/aicup2020-v${i}
        break
    fi
done
