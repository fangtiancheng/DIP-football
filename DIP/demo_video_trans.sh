# remove all the files in the output file folder
if [ ! -f "./inputfile/demo.mp4" ]; then
    echo "file ./inputfile/demo.mp4 does not exist"
    echo "exit 1"
    exit 1
fi
rm -r ./trans_out/*

# conda activate yolo

cd ./sportsfield_release-master/
# (yolo)
PYTHON_PATH=/home/ftc/.conda/envs/yolo/bin/python
$PYTHON_PATH get_first_pic.py

cd ..

# conda deactivate 

# conda activate sportsfield

cd ./sportsfield_release-master

# (sportsfield)
PYTHON_PATH=/home/ftc/.conda/envs/sportsfield/bin/python
$PYTHON_PATH ./fileld_trans_copy.py

cd ..

# conda deactivate

tar -cvf trans_out.tar ./trans_out
echo "finished"