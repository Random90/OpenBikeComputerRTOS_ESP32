/*
Use this script to convert microC array generated in GLCD font creator to proper C binary array compatibile with pcd8544 display driver library
*/

string = "00000000,00000000,00000000,00000000,11100000,00000001,11100000,00000001,11100000,00000001,11111110,00000001,11111110,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11111110,00011111,11111110,00011111,00000000,00000000,00000000,00000000,00000000,00000000";

const COLUMNS = 2;

let buffer = [['']];
let columnsCounter = 0;
let row = 0;

fillTheBuffer();
mirrorAllBytes();
rotateBmpClockWise90();
prependBinarySign();
const result = asString();
console.log(result);
return result;


function fillTheBuffer() {
  for (let i = 0; i < string.length; i++) {
    if (string[i] === ',') {
      if (columnsCounter === COLUMNS - 1) {
        columnsCounter = 0;
        row++;
        buffer[row] = [[]];
      } else {
        columnsCounter++;
        buffer[row][columnsCounter] = '';
      }
    } else {
      buffer[row][columnsCounter] += string[i];
    }
  }
}

function mirrorAllBytes() {
  [...Array(row)].forEach((_, i) => {
    [...Array(COLUMNS)].forEach((_, j) => {
      buffer[i][j] = mirrorByte(buffer[i][j]);
    });
  });
}

function prependBinarySign() {
  buffer = buffer.map((value) => '0b' + value);
}

function mirrorByte(byteStr) {
  let buffer = '';
  for (let i = 7; i >= 0; i--) {
    buffer += byteStr[i];
  }
  return buffer;
}
// TODO fill buffer columns if row mod 8 > 0 to keep whole image
// now it will be cropped due to library limitations
// use bitwise operators?
function rotateBmpClockWise90() {
  // take first 8 bytes from first column
  // rotate, update rotationMap
  // switch column and take first 8 bytes
  // rotate, update rotationMap
  // repeat until every entry on the array is > rows - switches mod 8
  // add zeroes for calculated difference from above
  // size of rotated bitmap = oldCols * 8bits [16] x nr of repeats on each col [3]
  // pushing to rotationBuffer is sufficient, resulting 1d array considers pcd library rendering algorithm
  let colIdx = 0;
  let byteBuffer = [];
  let rotatedBlockBuffer = [];
  for (let i = 0; i < buffer.length; i++) {
    // load 8 bytes from buffer
    byteBuffer.push(buffer[i][colIdx]);
    if (i % 8 === 7) {
      // rotate loaded column and push into final array
      byteBuffer.forEach((byteStr, byteIdx) => {
        for (let bitIdx = 0; bitIdx < 8; bitIdx++) {
          if (!rotatedBlockBuffer[bitIdx]) {
            rotatedBlockBuffer[bitIdx] = '';
          }
          rotatedBlockBuffer[bitIdx] = byteStr[bitIdx] + rotatedBlockBuffer[bitIdx];
        }
      });
      rotatedBitmap = rotatedBitmap.concat(rotatedBlockBuffer);
      rotatedBlockBuffer = [];
      byteBuffer = [];
      // switch buffer column
      if (colIdx < COLUMNS - 1) {
        colIdx++;
        i -= 8;
      } else {
        colIdx = 0;
      }
    }
  }
  buffer = rotatedBitmap;
}

function asString() {
  let resultStr = '';
  buffer.forEach((byte, i) => {
    resultStr += byte + (i === buffer.length - 1 ? ' ' : ',\n');
  })
  return resultStr;
}