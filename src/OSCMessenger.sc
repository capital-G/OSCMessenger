OSCMessenger : UGen {
	*kr {
		^this.multiNew('control');
	}

	checkInputs {
		^this.checkValidInputs;
	}
}
