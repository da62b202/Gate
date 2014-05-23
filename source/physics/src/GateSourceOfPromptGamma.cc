/*----------------------
  GATE version name: gate_v7

  Copyright (C): OpenGATE Collaboration

  This software is distributed under the terms
  of the GNU Lesser General  Public Licence (LGPL)
  See GATE/LICENSE.txt for further details
  ----------------------*/

#include "GateConfiguration.h"
#include "GateSourceOfPromptGamma.hh"
#include "GateRandomEngine.hh"
#include "G4ParticleTable.hh"
#include "G4Event.hh"
#include "G4Gamma.hh"

//------------------------------------------------------------------------
GateSourceOfPromptGamma::GateSourceOfPromptGamma(G4String name)
  :GateVSource( name )
{
  pMessenger = new GateSourceOfPromptGammaMessenger(this);
  // Create data object (will be initialized later)
  mData = new GateSourceOfPromptGammaData;
  mIsInitializedFlag = false;
  mFilename = "no filename given";
}
//------------------------------------------------------------------------


//------------------------------------------------------------------------
GateSourceOfPromptGamma::~GateSourceOfPromptGamma()
{
  delete pMessenger;
}
//------------------------------------------------------------------------


//------------------------------------------------------------------------
void GateSourceOfPromptGamma::SetFilename(G4String filename)
{
  mFilename = filename;
}
//------------------------------------------------------------------------


//------------------------------------------------------------------------
void GateSourceOfPromptGamma::Initialize()
{
  DD("GateSourceOfPromptGamma::Initialize");

  // Get pointer to the random engine
  CLHEP::HepRandomEngine * engine = GateRandomEngine::GetInstance()->GetRandomEngine();
  DD(engine);
  // engine->showStatus();

  CLHEP::HepRandomEngine * ee = CLHEP::HepRandom::getTheEngine();
  DD(ee);
  // ee->showStatus();
  // mData->SetRandomEngine(engine); FIXME ? Necessary or not ?

  // Get filename, load data
  mData->LoadData(mFilename);

  // Compute cmulative marginals information
  mData->Initialize();

  // Particle type is photon. Could not be initialize here.

  // Weight is fixed for the moment (could change in the future)
  //particle_weight = 1.0; // could not be initialized here

  // It is initialized
  mIsInitializedFlag = true;
}
//------------------------------------------------------------------------


//------------------------------------------------------------------------
void GateSourceOfPromptGamma::GenerateVertex(G4Event* aEvent)
{
  // Initialisation of the distribution information (only once)
  if (!mIsInitializedFlag) Initialize();

  // Position
  G4ThreeVector particle_position;
  mData->SampleRandomPosition(particle_position);

  // The position coordinate is expressed in the coordinate system
  // (CS) of the volume it was attached to during the TLEActor
  // simulation. Now we convert the coordinates into world
  // coordinates.
  ChangeParticlePositionRelativeToAttachedVolume(particle_position);

  // Energy
  mData->SampleRandomEnergy(mEnergy);

  // Direction
  G4ParticleMomentum particle_direction;
  mData->SampleRandomDirection(particle_direction);
  ChangeParticleMomentumRelativeToAttachedVolume(particle_direction);

  // Momentum
  double mass = GetParticleDefinition()->GetPDGMass();
  double pmom = std::sqrt(mEnergy*mEnergy-mass*mass);
  double d = std::sqrt(pow(particle_direction[0],2) +
                       pow(particle_direction[1],2) +
                       pow(particle_direction[2],2));
  double px = pmom * particle_direction[0]/d;
  double py = pmom * particle_direction[1]/d;
  double pz = pmom * particle_direction[2]/d;

  // Create vertex
  G4PrimaryParticle* particle =
    new G4PrimaryParticle(G4Gamma::Gamma(), px, py, pz);
  G4PrimaryVertex* vertex;
  vertex = new G4PrimaryVertex(particle_position, GetParticleTime());
  vertex->SetWeight(1.0); // FIXME
  vertex->SetPrimary(particle);
  aEvent->AddPrimaryVertex(vertex);
}
//------------------------------------------------------------------------


//------------------------------------------------------------------------
G4int GateSourceOfPromptGamma::GeneratePrimaries(G4Event* event)
{
  GenerateVertex(event);
  G4PrimaryParticle  * p = event->GetPrimaryVertex(0)->GetPrimary(0);
  GateMessage("Beam", 3, "(" << event->GetEventID() << ") " << p->GetG4code()->GetParticleName()
              << " pos=" << event->GetPrimaryVertex(0)->GetPosition()
              << " weight=" << p->GetWeight()
              << " energy=" << G4BestUnit(mEnergy, "Energy")
              << " mom=" << p->GetMomentum()
              << " ptime=" <<  G4BestUnit(p->GetProperTime(), "Time")
              << " atime=" <<  G4BestUnit(GetTime(), "Time")
              << ")" << G4endl);
  return 1; // a single vertex
}
//------------------------------------------------------------------------
