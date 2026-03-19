#include "xparameters.h"
#include "xil_types.h"
#include <iostream>

using namespace std;

// Find the timer base address macro (varies by export)
#if defined(XPAR_TMRCTR_0_BASEADDR)
  #define TMR_BASEADDR XPAR_TMRCTR_0_BASEADDR
#elif defined(XPAR_AXI_TIMER_0_BASEADDR)
  #define TMR_BASEADDR XPAR_AXI_TIMER_0_BASEADDR
#else
  #error "Timer base address macro not found"
#endif

// AXI Timer assumed clock per lab example
static const double CLK_HZ = 50000000.0;

// TCSR bit positions from DS764 tables (Timer0 and Timer1 are similar)
static const u32 BIT_MDT   = (1u << 0);   // mode: 0 generate, 1 capture
static const u32 BIT_UDT   = (1u << 1);   // 0 up, 1 down
static const u32 BIT_GENT  = (1u << 2);   // enable external generate signal
static const u32 BIT_ARHT  = (1u << 4);   // auto reload/hold
static const u32 BIT_LOAD  = (1u << 5);   // load TCR from TLR
static const u32 BIT_ENT   = (1u << 7);   // enable timer
static const u32 BIT_ENALL = (1u << 10);  // enable all timers
static const u32 BIT_PWM   = (1u << 9);   // PWM enable

static u32 seconds_to_tlr(double seconds)
{
    // counts = seconds * CLK_HZ. TLR = counts - 2 (per lab formula)
    if (seconds < 0.0) seconds = 0.0;
    double counts_d = seconds * CLK_HZ;
    if (counts_d < 2.0) counts_d = 2.0;

    // Clamp to 32-bit range
    if (counts_d > 4294967295.0) counts_d = 4294967295.0;

    u32 counts = (u32)(counts_d);
    return (counts >= 2u) ? (counts - 2u) : 0u;
}

int main()
{
    volatile u32* T = (volatile u32*)TMR_BASEADDR;

    // Register map (word indices)
    // 0: TCSR0   1: TLR0   2: TCR0
    // 4: TCSR1   5: TLR1   6: TCR1

    cout << "#### Lab3 Part 3: PWM Mode (PWM0 -> D15) ####" << endl;
    cout << "Enter period (seconds) and ON-time (seconds). Example: 4 then 2 gives 50% duty." << endl;

    while (true)
    {
        double period_s = 0.0;
        double on_s = 0.0;

        cout << "\nPeriod (s): ";
        cin >> period_s;
        cout << "On-time (s): ";
        cin >> on_s;

        if (period_s <= 0.0)
        {
            cout << "Period must be > 0." << endl;
            continue;
        }
        if (on_s < 0.0) on_s = 0.0;
        if (on_s > period_s) on_s = period_s;

        // Stop both timers first (leave mode bits configured)
        T[0] = 0;
        T[4] = 0;

        // Configure PWM mode, generate mode, down-count, auto reload, enable generate output
        // MDT = 0 (generate), UDT = 1 (down), GENT = 1, ARHT = 1, PWM = 1
        u32 cfg0 = BIT_PWM | BIT_ARHT | BIT_GENT | BIT_UDT; // MDT bit left 0
        u32 cfg1 = BIT_PWM | BIT_ARHT | BIT_GENT | BIT_UDT;

        // Compute and write load registers
        u32 tlr0 = seconds_to_tlr(period_s);
        u32 tlr1 = seconds_to_tlr(on_s);

        cout << "Computed: TLR0=" << tlr0 << "  TLR1=" << tlr1 << endl;

        T[1] = tlr0; // TLR0
        T[5] = tlr1; // TLR1

        // Load TLR -> TCR (set LOAD bit), then clear LOAD so timer can run
        T[0] = cfg0 | BIT_LOAD;
        T[4] = cfg1 | BIT_LOAD;

        T[0] = cfg0;
        T[4] = cfg1;

        // Start both timers
        // ENALL helps start both, ENT bits ensure each timer runs
        T[0] = cfg0 | BIT_ENALL | BIT_ENT;
        T[4] = cfg1 | BIT_ENALL | BIT_ENT;

        cout << "PWM running. LED on D15 should blink with ON " << on_s << "s and OFF " << (period_s - on_s) << "s." << endl;
    }

    return 0;
}
