use crate::helpers::HexFmt;
use crate::{Container, Parse, Stream};

pub struct PatchFile {
    pub signature: [u8; 8],
    pub unknown0: u32,
    pub containers: Vec<Container>,
}

impl Parse for PatchFile {
    fn parse(stream: &mut impl Stream) -> std::io::Result<Self> {
        use byteorder::{ReadBytesExt, BE};
        let mut signature = [0; 8];
        stream.read_exact(&mut signature)?;
        let unknown0 = stream.read_u32::<BE>()?;

        let mut containers = vec![];
        loop {
            let container = Container::parse(stream)?;
            containers.push(container);
            if &container.signature == b"EOF_" {
                break;
            }
        }

        Ok(PatchFile {
            signature,
            unknown0,
            containers,
        })
    }
}

impl std::fmt::Debug for PatchFile {
    fn fmt(&self, fmt: &mut std::fmt::Formatter) -> std::fmt::Result {
        let sig = String::from_utf8_lossy(&self.signature);
        fmt.debug_struct("Container")
            .field("signature", &sig)
            .field("unknown0", &HexFmt(&self.unknown0))
            .field("containers", &self.containers)
            .finish()
    }
}
