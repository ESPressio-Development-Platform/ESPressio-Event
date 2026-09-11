"""Compile public headers, maintained examples and semantic rejection contracts."""
import argparse,re,subprocess,tempfile
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--dependencies',type=Path,required=True);a=p.parse_args()
r=Path(__file__).resolve().parents[1];deps=a.dependencies.resolve()
flags=['g++','-std=c++17','-Wall','-Wextra','-Werror','-fno-rtti',f'-I{r/"src"}']
flags += [f'-I{deps/("ESPressio-"+n)/"src"}' for n in ['System','Primitive','Task','Threads','Timing','Serializable','Units','Observable']]
flags += [f'-I{deps/"ESPressio-Timing/tests/stubs"}']
with tempfile.TemporaryDirectory() as d:
    d=Path(d)
    def compile(name,source,rejection=None):
        f=d/(name+'.cpp');f.write_text(source)
        result=subprocess.run(flags+['-c',str(f),'-o',str(d/(name+'.o'))],text=True,capture_output=True)
        if rejection is None:
            if result.returncode: raise RuntimeError(name+'\n'+result.stderr)
        elif result.returncode==0 or rejection not in result.stderr:
            raise RuntimeError('Expected '+rejection+' for '+name+'\n'+result.stderr)
        print(name+' passed',flush=True)
    for h in sorted((r/'src').glob('*.hpp')): compile(h.stem,f'#include <{h.name}>\n')
    for i,s in enumerate(re.findall(r'```cpp\n(.*?)```',(r/'README.md').read_text(),re.S)): compile('readme'+str(i),s)
    for f in (r/'examples').rglob('*.ino'): compile(f.stem,f.read_text())
    base='#include <ESPressio_Event.hpp>\nnamespace E=ESPressio::Event;\n'
    definition='struct A:E::Event<A>{static constexpr E::EventTypeId TypeId{1};static constexpr int MaximumLiveInstances=2,MaximumPendingInstances=1;};\n'
    compile('zero_id',base+definition.replace('TypeId{1}','TypeId{0}')+'E::EventTypeRuntime<A> x;', 'must be nonzero')
    compile('zero_pool',base+definition.replace('MaximumLiveInstances=2','MaximumLiveInstances=0')+'E::EventTypeRuntime<A> x;', 'must be positive')
    compile('negative_pending',base+definition.replace('MaximumPendingInstances=1','MaximumPendingInstances=-1')+'E::EventTypeRuntime<A> x;', 'cannot be negative')
    compile('untyped_id',base+definition.replace('E::EventTypeId TypeId{1}','int TypeId=1')+'E::EventTypeRuntime<A> x;', 'strong EventTypeId')
    compile('empty_capability',base+'E::ThreadCapability<E::SharedPendingCapacity<1>> x;', 'requires declared Types')
    compile('duplicate_type',base+definition+'E::ThreadCapability<E::SharedPendingCapacity<0>,A,A> x;', 'Types must be unique')
    compile('shared_missing',base+definition.replace('MaximumPendingInstances=1','MaximumPendingInstances=0')+'E::ThreadCapability<E::SharedPendingCapacity<0>,A> x;', 'shared')
    compile('lease_copy',base+'void f(const E::EventLease& a){E::EventLease b=a;}', 'deleted')
    compile('local_outbound',base+'#include <ESPressio_EventOutboundBinding.hpp>\n'+definition+'E::EventOutboundBinding<A> x;', 'require a Transmissible Type')
    compile('undeclared_listener',base+definition+'struct B:A{}; struct O{void Receive(const B&) {}}; void f(){O o; E::ThreadCapability<E::SharedPendingCapacity<0>,A> x;x.Listen<B>(o,&O::Receive);}', 'declared Event Type')

    transmit=base+definition.replace('E::Event<A>','E::TransmissibleEvent<A>')
    compile('missing_policy',transmit+'static_assert(A::ValidateTier());', 'DeliveryPolicy')
    unbounded=transmit[:-3]+( 'std::string Value; ESPRESSIO_SERIALIZABLE_TYPE(A) ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1) ESPRESSIO_SERIALIZABLE_PROPERTIES(ESPRESSIO_PROPERTY("value",Value)) };\n')
    compile('unbounded_wire',unbounded+'static_assert(A::ValidateTier());', 'bounded P3 schema')
