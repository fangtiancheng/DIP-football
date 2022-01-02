# remove all the files in the output file folder
rm -rf ./output/*
rm -rf ./Yolov5_DeepSort_Pytorch/inference/output

mkdir ./output/json

# into the first directory
# conda activate activity2vec
cd ./HAKE-Action-Torch-Activity2Vec
PYTHON_PATH=/home/ftc/.conda/envs/activity2vec/bin/python
$PYTHON_PATH -u tools/demo.py --cfg configs/a2v/a2v.yaml  --input ../inputfile/demo.mp4 --mode video   --output ../output --save-res --save-vis
cd ..
# conda deactivate

# into the second directory
# conda activate yolo
PYTHON_PATH=/home/ftc/.conda/envs/yolo/bin/python
cd ./Yolov5_DeepSort_Pytorch
$PYTHON_PATH ./track.py --source ../inputfile/demo.mp4 --save-vid --save-txt --classes 0
cp ./inference/output/demo.txt ../output
cd ..
# conda deactivate

# conda activate yolo

cd ./DIP-utils

$PYTHON_PATH refine_network.py

cd ..

# conda deactivate
echo "finished"