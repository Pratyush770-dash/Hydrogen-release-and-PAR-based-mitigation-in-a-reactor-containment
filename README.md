# Hydrogen-release-and-PAR-based-mitigation-in-a-reactor-containment
A transient species-transport CFD study in ANSYS Fluent, modelling a low-rate hydrogen release into a sealed enclosure fitted with a Passive Autocatalytic Recombiner (PAR), with the recombiner's reaction rate taken directly from the AREVA FR-380 vendor correlation used in the THAI HR2/HR5 benchmark (Halouane & Dehbi, 2018).

I built this after some exposure to PAR placement research, and wanted to see if I could put together the full species-transport + buoyancy + custom-reaction-UDF workflow myself, end to end, rather than just reading about how it's done. It turned out to be a much longer project than I expected, mostly because of meshing and connectivity issues between the PAR housing and the main domain — more on that below.

Table of Contents
- [What was actually done](#what-was-actually-done)
- [Setup / Software Used](#setup--software-used)
- [Reproducing the Results](#reproducing-the-results)
- [Results](#results)
- [Method, briefly](#method-briefly)
- [Limitations](#limitations)
- [Repository structure](#repository-structure)
- [Author](#author)

## What was actually done

- Built a simplified 5 m × 5 m × 8 m rectangular enclosure in DesignModeler as a stand-in for the THAI facility's cylindrical vessel (3.2 m diameter × 9.2 m height) — chosen for geometric simplicity, not as a literal reproduction
- Modelled a small PAR housing (0.4 m × 1 m × 0.15 m) as its own body, merged into the main fluid domain so the inlet/outlet faces could pass flow between the PAR and the room
- Implemented the actual AREVA FR-380 hydrogen recombination correlation from Halouane & Dehbi (2018) as a Fluent UDF, rather than using a flat or guessed sink rate — the UDF samples H2/O2 concentration at the PAR inlet face every iteration and applies the resulting sink to the housing's cell zone
- Set the release rate to 0.157 g/s, matching the real THAI HR2 test condition, with pure H2 at ambient temperature
- Ran a transient species-transport case with buoyancy (ideal-gas density, gravity along the vessel's vertical axis) and tracked H2 concentration at several monitor points through the room
- Spent a fair amount of time debugging why the PAR outlet face wouldn't register as a proper interior connection to the room — eventually resolved with a manual Mesh Interface once the usual Share Topology / Form New Part route didn't fully take
- Reduced the mesh from an initial 2.4 million elements (which exceeded the Student license limit) down to under 900k by loosening bulk sizing while keeping the release and PAR faces finely resolved

## Setup / Software Used

* ANSYS DesignModeler (geometry)
* ANSYS Meshing — MultiZone method on the main enclosure, patch-conforming tetrahedral method on the PAR housing
* ANSYS Fluent 2026 R1, Student version (solver, species transport, custom UDF)
* Fluent's native XY-Plot tools for monitor-point tracking

## Reproducing the Results

1. Build the enclosure as a 5 × 5 × 8 m extruded solid in DesignModeler, sketched on the base plane and extruded upward
2. Imprint a small circular face low on one wall for the release inlet
3. Build the PAR housing as a separate 0.4 × 1 × 0.15 m box on a new offset plane, positioned on the same wall, then select both bodies and use Form New Part so the shared faces mesh conformally
4. Mesh: global size 0.9 m, face sizing 0.05 m at the release inlet and 0.03 m at the PAR faces, MultiZone on the room body, patch-conforming on the PAR body
5. In Fluent, enable Species Transport and Energy, add h2, h2o, o2, n2 to the mixture, set density to ideal-gas
6. Load the PAR reaction UDF (interpreted, not compiled) and hook it to the PAR housing's h2 source term
7. Set release_inlet as a mass-flow-inlet at 0.157 g/s, pure H2, and the room walls to a convective boundary at h = 50 W/m²K, matching the oil-bath condition described in the benchmark paper
8. Solve transient, SIMPLE scheme, second-order discretisation, 0.01 s time step

## Results

Flat-out, the biggest constraint on this project was compute time on a personal laptop, not the setup itself. The run reached about 20 seconds of simulated flow time before I had to stop for a deadline — nowhere near the roughly 1200 seconds the real HR2 test needed before the PAR even starts recombining meaningfully. So what's here is the mixing and transport phase, not the full reaction phase.

Even so, the trend that came out is the right one. H2 concentration at the mid-compartment monitor starts climbing around t = 12 s, the PAR-region monitor picks up a few seconds later around t = 16–17 s, and the ceiling monitor only shows a late uptick right near the end of the run, around t = 18–20 s. That ordering — middle of the room filling before the ceiling does — is what you'd expect from a buoyant plume spreading outward from a low release point before it's had time to fully stratify toward the top of the vessel.

One monitor point (near the release inlet itself) is left out of the results — I never went back and matched its coordinates precisely to the final geometry after a few redesigns, so the trace it produced was flat and not trustworthy. Rather than include a misleading plot, I've dropped it.

## Method, briefly

- **Geometry:** rectangular enclosure, simplified relative to the cylindrical THAI vessel in the benchmark literature. PAR housing built as a separate small body and merged into the main part.
- **Mesh:** mixed MultiZone/tetrahedral approach, with local refinement at the release and PAR faces and a coarser bulk mesh elsewhere to stay under the element-count ceiling.
- **Species and buoyancy:** species transport with h2, h2o, o2, n2, ideal-gas density, gravity enabled, operating density set to zero per Fluent's own guidance for buoyant variable-density flows.
- **PAR reaction:** implemented from the actual vendor correlation in the benchmark paper, not approximated — including the oxygen-starvation correction term and a clip at low concentration, since the raw formula goes slightly negative below the reaction onset threshold, which isn't physical.
- **Solver:** pressure-based, transient, SIMPLE, second-order upwind, 0.01 s time step.

## Limitations

The run did not reach far enough in simulated time for the PAR's recombination term to meaningfully activate — the benchmark test needs roughly 1200 s before hydrogen at the PAR inlet crosses the threshold where recombination starts, and this run only covers 20 s. What's validated here is the transport and mixing physics, not the recombination dynamics.

The enclosure is a simplified rectangular box, not the real cylindrical THAI vessel, so comparison against HR2 is qualitative rather than a direct reproduction.

After merging the PAR housing and the room into a shared part to fix a mesh connectivity issue, all six faces of the PAR housing ended up registering as interior rather than just the intended top and bottom. In the real device and in the benchmark paper's model, the PAR is a sealed duct with only two openings; here it behaves more like an open reaction pocket sitting inside the room. I wasn't able to fully resolve this within the time available, and I'm noting it directly rather than glossing over it.

The PAR outlet face needed a manually created Mesh Interface rather than a clean conformal connection, after Share Topology didn't fully take on that one face. It appears to be functioning correctly based on the console output, but it's a workaround rather than the cleaner geometric fix I'd have preferred.

Only one release-rate case was run. The original plan — three PAR positions × three release rates × three ventilation states — was not completed; this submission covers a single baseline case.

No formal grid-independence study was performed, given the time constraints; mesh quality was checked qualitatively (orthogonal quality, skewness) rather than through a full multi-mesh comparison.

## Repository structure

```
├── README.md
├── geometry/
│   └── geometry
├── mesh/
│   └── mesh
├── udf/
│   └── par_source.c   → PAR reaction UDF (AREVA FR-380 correlation)
├── results/
│   ├── h2_monitors/   → facet-average H2 plots at mid-compartment, PAR, ceiling
└── case_files/
    └── Hydrogen_PAR_enclosure_case
```

## Author

**Pratyush Dash**

B.Tech Chemical Engineering, KIIT University, Bhubaneswar
