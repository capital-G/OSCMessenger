OSCMessenger : UGen {
	*kr {|portNumber, oscAddress, values, trigger=60, doneAddress=nil, doneValue=1|
		if(values.containsSeqColl.not) { values = values.bubble };
		if(trigger.isFloat.or(trigger.isInteger)) { trigger = Impulse.kr(trigger) };
		trigger = trigger.bubble;
		[portNumber, oscAddress, values, trigger, doneAddress, doneValue].flop.do { |args|
			this.new1('control', *args);
		};
	}

	checkInputs {
		^this.checkValidInputs;
	}

	*new1 {|rate, portNumber, oscAddress, values, trigger, doneAddress, doneValue|
		var oscAddressAscii;
		var doneAddressAscii;
		var args;

		doneAddress = if(doneAddress.isNil, {""}, {doneAddress}).asString;

		oscAddressAscii = oscAddress.ascii;
		doneAddressAscii = doneAddress.ascii;

		args = [
			rate,
			portNumber,
			trigger,
			oscAddressAscii.size,
			doneAddressAscii.size,
			doneValue,
		].addAll(oscAddressAscii).addAll(doneAddressAscii).addAll(values);

		^super.new1(*args);
	}
}
