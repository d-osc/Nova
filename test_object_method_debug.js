// Debug object methods
const obj = {
    value: 42,
    getValue: function() {
        console.log("In getValue, this =", this);
        console.log("In getValue, this.value =", this.value);
        return this.value;
    }
};

console.log("obj.value =", obj.value);
const result = obj.getValue();
console.log("obj.getValue() returned:", result);
