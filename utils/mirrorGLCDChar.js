string = "00000000,00000000,00000000,00000000,11100000,00000001,11100000,00000001,11100000,00000001,11111110,00000001,11111110,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11100000,00000001,11111110,00011111,11111110,00011111,00000000,00000000,00000000,00000000";

const COLUMNS = 2;

let buffer = [['']];
let columnsCounter = 0;
let row = 0;

fillTheBuffer();
mirrorAllBytes();
rotateBmpClockWise90();
parseColumnsLastRowsFirst();
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

function parseColumnsLastRowsFirst() {
  let newBuffer = [];
  for (let j = COLUMNS - 1; j >= 0; j--) {
    [...Array(row)].forEach((_, i) => {
      newBuffer.push(buffer[i][j]);
    });
  }
  buffer = newBuffer;
}

function mirrorByte(byteStr) {
  let buffer = '';
  for (let i = 7; i >= 0; i--) {
    buffer += byteStr[i];
  }
  return buffer;
}

// use bitwise operators?
function rotateBmpClockWise90() {
  let rotatedBuffer = [];
  let byteCache = [];

  let rotationMap = new Array[COLUMNS];
  rotationMap.fill(0);
  // TODO use map to switch columns after 8 bytes
  // take first 8 bytes from first column
  // rotate, update rotationMap
  // switch column and take first 8 bytes
  // rotate, update rotationMap
  // repeat until every entry on the array is > rows - switches mod 8
  // add zeroes for calculated difference from above
  // size of rotated bitmap = oldCols * 8bits [16] x nr of repeats on each col [3]
  // pushing to rotationBuffer is sufficient, resulting 1d array considers pcd library rendering algorithm


  // if (byteCache.length < 8) {
  //   // cache next 8 bytes
  //   byteCache.push(byteArr[j]);
  // }

  //   byteCache.forEach((byteStr, cachedIdx) => {
  //     for (let bitIdx = 0; bitIdx < 8; bitIdx++) {
  //       if (!rotatedBuffer[bitIdx]) {
  //         rotatedBuffer[bitIdx] = '';
  //       }
  //       rotatedBuffer[bitIdx] = byteStr[bitIdx] + rotatedBuffer[bitIdx];
  //     }
  //   });




  buffer = rotatedBuffer;
}

function asString() {
  let resultStr = '';
  buffer.forEach((byte, i) => {
    resultStr += byte + (i === buffer.length - 1 ? ' ' : ',\n');
  })
  return resultStr;
}