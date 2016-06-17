#ifndef FASTSIM_INTERACTIONMODEL
#define FASTSIM_INTERACTIONMODEL

#include "string"
#include "vector"

namespace edm
{
    class Event;
    class ProducerBase;
}

class RandomEngineAndDistribution;

namespace fastsim
{
    class Layer;
    class Particle;
    class InteractionModel
    {
    public:
	InteractionModel(std::string className)
	    : className_(className){;}
	virtual ~InteractionModel(){;}
	virtual void interact(Particle & particle,const Layer & layer,std::vector<Particle> & secondaries,const RandomEngineAndDistribution & random) = 0;
	virtual void registerProducts(edm::ProducerBase & producer) const{;}
	virtual void storeProducts(edm::Event & iEvent) {;}
 	friend std::ostream& operator << (std::ostream& o , const InteractionModel & model); 
   private:
	const std::string className_;
    };
    std::ostream& operator << (std::ostream& os , const InteractionModel & interactionModel);

}

#endif
