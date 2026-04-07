# TODO: Maybe we could add the asset handling here

# -----------------------------
# Parse arguments
# Usage: ./build.sh gd=../my_game od=out clean=0
# -----------------------------
for arg in "$@"; do
  case $arg in
  gd=*)
    GAME_DIR="${arg#*=}"
    ;;
  od=*)
    OUTPUT_DIR="${arg#*=}"
    ;;
  clean=*)
    CLEAN="${arg#*=}"
    ;;
  *)
    echo "Unknown option: $arg"
    exit 1
    ;;
  esac
done


if [ -z $GAME_DIR ]; then
  GAME_DIR="./src/demo"
  ENGINE_DIR="./"
  OUTPUT_DIR="./build"
  CLEAN=1
fi

# -----------------------------
# Prepare output directory
# -----------------------------

if [ "$CLEAN" -eq 1 ]; then
  rm -rf "$OUTPUT_DIR"
fi

mkdir -p "$OUTPUT_DIR"

