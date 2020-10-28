SCRIPT_DIR=$(cd $(dirname $0) && pwd)
BUILD_DIR=$SCRIPT_DIR/build

echo "SCRIPT_DIR= $SCRIPT_DIR"
echo "BUILD_DIR= $BUILD_DIR"

mkdir $BUILD_DIR

cmake "$SCRIPT_DIR" -B"$BUILD_DIR" -DCMAKE_BUILD_TYPE=$1
cmake  --build "$BUILD_DIR" --config $1 -- -j4
