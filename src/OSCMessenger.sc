OSCMessenger : UGen {
	*kr {|portNumber, oscAddress|
		^this.new1('control', portNumber, oscAddress);
	}

	checkInputs {
		^this.checkValidInputs;
	}

	*new1 {|rate, portNumber, oscAddress|
		var ascii = oscAddress.ascii;
		^super.new1(*[rate, portNumber, ascii.size].addAll(ascii));
	}
}
