<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet version="1.0"
xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:output method="text" indent="yes" encoding="utf-8" standalone="yes"/>
<xsl:template match="/">

	<xsl:text>File	proportion
</xsl:text>
      <xsl:for-each select="root/entry">

        <xsl:sort data-type="number" order="descending" select="assembled/proportion" />

<xsl:if test="demultiplexedKmerObservations/text() != '0'">

<xsl:value-of select="file"/>

	<xsl:text>	</xsl:text>
          	<xsl:value-of select="assembled/proportion"/>

	<xsl:text>
</xsl:text>

</xsl:if>

      </xsl:for-each>
</xsl:template>

</xsl:stylesheet> 
