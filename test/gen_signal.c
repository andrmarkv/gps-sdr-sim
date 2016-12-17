/*
 * This is main function that is supposed to generate
 * signals for the given motion_path and store to the file
 */
void* gen_signal(t_sim_context *context, t_motion *motion) {
	clock_t tstart = clock();
	int iumd = 0;
	int i = 0;
	int sv = 0;
	double path_loss;
	double ant_gain;
	int gain[MAX_CHAN];

	ephem_t eph[][] = context->eph; //reassign for the simplicity
	channel_t chan[] = context->chan; //reassign for the simplicity
	gpstime_t grx = context->grx; //reassign for the simplicity
	int ieph = context->ieph; //reassign for the simplicity
	ionoutc_t ionoutc = *(context->ionoutc); //reassign for the simplicity
	double elvmask = context->elvmask; //reassign for the simplicity

	// Update receiver time
	grx = incGpsTime(grx, 0.1);

	/*
	 * TODO: This for has to be replaced with while loop iterating
	 * over the list of motion
	 * numd - will not be needed
	 * iumd - will not be needed
	 */
	for (iumd = 1; iumd < context->numd; iumd++) {

		/* Satellites loop */
		for (i = 0; i < MAX_CHAN; i++) {
			if (chan[i].prn > 0) {
				// Refresh code phase and data bit counters
				range_t rho;
				sv = chan[i].prn - 1;

				// Current pseudorange
				computeRange(&rho, eph[ieph][sv], &ionoutc, grx, xyz[iumd]);
				chan[i].azel[0] = rho.azel[0];
				chan[i].azel[1] = rho.azel[1];

				// Update code phase and data bit counters
				computeCodePhase(&chan[i], rho, 0.1);
				chan[i].carr_phasestep = (int) (512 * 65536.0 * chan[i].f_carr
						* delt);

				// Path loss
				path_loss = 20200000.0 / rho.d;

				// Receiver antenna gain
				ibs = (int) ((90.0 - rho.azel[1] * R2D) / 5.0); // covert elevation to boresight
				ant_gain = ant_pat[ibs];

				// Signal gain
				gain[i] = (int) (path_loss * ant_gain * 100.0); // scaled by 100
			}
		}

		int igrx = 0;
		for (int isamp = 0; isamp < iq_buff_size; isamp++) {
			int i_acc = 0;
			int q_acc = 0;
			int ip = 0;
			int qp = 0;

			for (i = 0; i < MAX_CHAN; i++) {
				if (chan[i].prn > 0) {
					iTable = (chan[i].carr_phase >> 16) & 511;

					ip = chan[i].dataBit * chan[i].codeCA * cosTable512[iTable]
							* gain[i];
					qp = chan[i].dataBit * chan[i].codeCA * sinTable512[iTable]
							* gain[i];

					i_acc += (ip + 50) / 100;
					q_acc += (qp + 50) / 100;

					// Update code phase
					chan[i].code_phase += chan[i].f_code * delt;

					if (chan[i].code_phase >= CA_SEQ_LEN) {
						chan[i].code_phase -= CA_SEQ_LEN;

						chan[i].icode++;

						if (chan[i].icode >= 20) { // 20 C/A codes = 1 navigation data bit
							chan[i].icode = 0;
							chan[i].ibit++;

							if (chan[i].ibit >= 30) { // 30 navigation data bits = 1 word
								chan[i].ibit = 0;
								chan[i].iword++;
								/*
								 if (chan[i].iword>=N_DWRD)
								 printf("\nWARNING: Subframe word buffer overflow.\n");
								 */
							}

							// Set new navigation data bit
							chan[i].dataBit =
									(int) ((chan[i].dwrd[chan[i].iword]
											>> (29 - chan[i].ibit)) & 0x1UL) * 2
											- 1;
						}
					}

					// Set currnt code chip
					chan[i].codeCA = chan[i].ca[(int) chan[i].code_phase] * 2
							- 1;

					// Update carrier phase
					chan[i].carr_phase += chan[i].carr_phasestep;
				}
			}

			// Store I/Q samples into buffer
			iq_buff[isamp * 2] = (short) i_acc;
			iq_buff[isamp * 2 + 1] = (short) q_acc;

		} // End of omp parallel for

		// Implement only data_format == SC16
		fwrite(iq_buff, 2, 2 * iq_buff_size, fp);

		//
		// Update navigation message and channel allocation every 30 seconds
		//

		igrx = (int) (grx.sec * 10.0 + 0.5);

		if (igrx % 300 == 0){ // Every 30 seconds

			// Update navigation message
			for (i = 0; i < MAX_CHAN; i++) {
				if (chan[i].prn > 0)
					generateNavMsg(grx, &chan[i], 0);
			}

			// Refresh ephemeris and subframes
			// Quick and dirty fix. Need more elegant way.
			for (sv = 0; sv < MAX_SAT; sv++) {
				if (eph[ieph + 1][sv].vflg == 1) {
					dt = subGpsTime(eph[ieph + 1][sv].toc, grx);
					if (dt < SECONDS_IN_HOUR) {
						ieph++;

						for (i = 0; i < MAX_CHAN; i++) {
							// Generate new subframes if allocated
							if (chan[i].prn != 0)
								eph2sbf(eph[ieph][chan[i].prn - 1], ionoutc,
										chan[i].sbf);
						}
					}

					break;
				}
			}

			// Update channel allocation
			allocateChannel(chan, eph[ieph], ionoutc, grx, xyz[iumd], elvmask);
		}

		// Update receiver time
		grx = incGpsTime(grx, 0.1);

		// Update time counter
		printf("\rTime into run = %4.1f", subGpsTime(grx, g0));
		fflush(stdout);
	}

}
