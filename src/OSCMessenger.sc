OSCMessenger : UGen {
	*kr {|portNumber, oscAddress, values|
		if(values.containsSeqColl.not) { values = values.bubble };
		[portNumber, oscAddress, values].flop.do { |args|
			this.new1('control', *args);
		};
	}

	checkInputs {
		^this.checkValidInputs;
	}

	*new1 {|rate, portNumber, oscAddress, values|
		var ascii = oscAddress.ascii;
		^super.new1(*[rate, portNumber, ascii.size].addAll(ascii).addAll(values));
	}
}
