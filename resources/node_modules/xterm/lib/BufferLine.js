"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
var Buffer_1 = require("./Buffer");
var CELL_SIZE = 3;
var IS_COMBINED_BIT_MASK = 0x80000000;
var BufferLine = (function () {
    function BufferLine(cols, fillCharData, isWrapped) {
        if (isWrapped === void 0) { isWrapped = false; }
        this.isWrapped = isWrapped;
        this._data = null;
        this._combined = {};
        if (!fillCharData) {
            fillCharData = [0, Buffer_1.NULL_CELL_CHAR, Buffer_1.NULL_CELL_WIDTH, Buffer_1.NULL_CELL_CODE];
        }
        if (cols) {
            this._data = new Uint32Array(cols * CELL_SIZE);
            for (var i = 0; i < cols; ++i) {
                this.set(i, fillCharData);
            }
        }
        this.length = cols;
    }
    BufferLine.prototype.get = function (index) {
        var stringData = this._data[index * CELL_SIZE + 1];
        return [
            this._data[index * CELL_SIZE + 0],
            (stringData & IS_COMBINED_BIT_MASK)
                ? this._combined[index]
                : (stringData) ? String.fromCharCode(stringData) : '',
            this._data[index * CELL_SIZE + 2],
            (stringData & IS_COMBINED_BIT_MASK)
                ? this._combined[index].charCodeAt(this._combined[index].length - 1)
                : stringData
        ];
    };
    BufferLine.prototype.getWidth = function (index) {
        return this._data[index * CELL_SIZE + 2];
    };
    BufferLine.prototype.set = function (index, value) {
        this._data[index * CELL_SIZE + 0] = value[0];
        if (value[1].length > 1) {
            this._combined[index] = value[1];
            this._data[index * CELL_SIZE + 1] = index | IS_COMBINED_BIT_MASK;
        }
        else {
            this._data[index * CELL_SIZE + 1] = value[1].charCodeAt(0);
        }
        this._data[index * CELL_SIZE + 2] = value[2];
    };
    BufferLine.prototype.insertCells = function (pos, n, fillCharData) {
        pos %= this.length;
        if (n < this.length - pos) {
            for (var i = this.length - pos - n - 1; i >= 0; --i) {
                this.set(pos + n + i, this.get(pos + i));
            }
            for (var i = 0; i < n; ++i) {
                this.set(pos + i, fillCharData);
            }
        }
        else {
            for (var i = pos; i < this.length; ++i) {
                this.set(i, fillCharData);
            }
        }
    };
    BufferLine.prototype.deleteCells = function (pos, n, fillCharData) {
        pos %= this.length;
        if (n < this.length - pos) {
            for (var i = 0; i < this.length - pos - n; ++i) {
                this.set(pos + i, this.get(pos + n + i));
            }
            for (var i = this.length - n; i < this.length; ++i) {
                this.set(i, fillCharData);
            }
        }
        else {
            for (var i = pos; i < this.length; ++i) {
                this.set(i, fillCharData);
            }
        }
    };
    BufferLine.prototype.replaceCells = function (start, end, fillCharData) {
        while (start < end && start < this.length) {
            this.set(start++, fillCharData);
        }
    };
    BufferLine.prototype.resize = function (cols, fillCharData) {
        if (cols === this.length) {
            return;
        }
        if (cols > this.length) {
            var data = new Uint32Array(cols * CELL_SIZE);
            if (this.length) {
                if (cols * CELL_SIZE < this._data.length) {
                    data.set(this._data.subarray(0, cols * CELL_SIZE));
                }
                else {
                    data.set(this._data);
                }
            }
            this._data = data;
            for (var i = this.length; i < cols; ++i) {
                this.set(i, fillCharData);
            }
        }
        else {
            if (cols) {
                var data = new Uint32Array(cols * CELL_SIZE);
                data.set(this._data.subarray(0, cols * CELL_SIZE));
                this._data = data;
                var keys = Object.keys(this._combined);
                for (var i = 0; i < keys.length; i++) {
                    var key = parseInt(keys[i], 10);
                    if (key >= cols) {
                        delete this._combined[key];
                    }
                }
            }
            else {
                this._data = null;
                this._combined = {};
            }
        }
        this.length = cols;
    };
    BufferLine.prototype.fill = function (fillCharData) {
        this._combined = {};
        for (var i = 0; i < this.length; ++i) {
            this.set(i, fillCharData);
        }
    };
    BufferLine.prototype.copyFrom = function (line) {
        if (this.length !== line.length) {
            this._data = new Uint32Array(line._data);
        }
        else {
            this._data.set(line._data);
        }
        this.length = line.length;
        this._combined = {};
        for (var el in line._combined) {
            this._combined[el] = line._combined[el];
        }
        this.isWrapped = line.isWrapped;
    };
    BufferLine.prototype.clone = function () {
        var newLine = new BufferLine(0);
        newLine._data = new Uint32Array(this._data);
        newLine.length = this.length;
        for (var el in this._combined) {
            newLine._combined[el] = this._combined[el];
        }
        newLine.isWrapped = this.isWrapped;
        return newLine;
    };
    BufferLine.prototype.getTrimmedLength = function () {
        for (var i = this.length - 1; i >= 0; --i) {
            if (this._data[i * CELL_SIZE + 1] !== 0) {
                return i + this._data[i * CELL_SIZE + 2];
            }
        }
        return 0;
    };
    BufferLine.prototype.copyCellsFrom = function (src, srcCol, destCol, length, applyInReverse) {
        var srcData = src._data;
        if (applyInReverse) {
            for (var cell = length - 1; cell >= 0; cell--) {
                for (var i = 0; i < CELL_SIZE; i++) {
                    this._data[(destCol + cell) * CELL_SIZE + i] = srcData[(srcCol + cell) * CELL_SIZE + i];
                }
            }
        }
        else {
            for (var cell = 0; cell < length; cell++) {
                for (var i = 0; i < CELL_SIZE; i++) {
                    this._data[(destCol + cell) * CELL_SIZE + i] = srcData[(srcCol + cell) * CELL_SIZE + i];
                }
            }
        }
        var srcCombinedKeys = Object.keys(src._combined);
        for (var i = 0; i < srcCombinedKeys.length; i++) {
            var key = parseInt(srcCombinedKeys[i], 10);
            if (key >= srcCol) {
                this._combined[key - srcCol + destCol] = src._combined[key];
            }
        }
    };
    BufferLine.prototype.translateToString = function (trimRight, startCol, endCol) {
        if (trimRight === void 0) { trimRight = false; }
        if (startCol === void 0) { startCol = 0; }
        if (endCol === void 0) { endCol = this.length; }
        if (trimRight) {
            endCol = Math.min(endCol, this.getTrimmedLength());
        }
        var result = '';
        while (startCol < endCol) {
            var stringData = this._data[startCol * CELL_SIZE + 1];
            result += (stringData & IS_COMBINED_BIT_MASK) ? this._combined[startCol] : (stringData) ? String.fromCharCode(stringData) : Buffer_1.WHITESPACE_CELL_CHAR;
            startCol += this._data[startCol * CELL_SIZE + 2] || 1;
        }
        return result;
    };
    return BufferLine;
}());
exports.BufferLine = BufferLine;
//# sourceMappingURL=BufferLine.js.map