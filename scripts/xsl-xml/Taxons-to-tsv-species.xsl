<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0"
xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="text" indent="yes" encoding="utf-8" standalone="yes"/>
<xsl:strip-space elements="*"/>
<xsl:template match="/">

<xsl:text>#Rank	Taxon	Colored k-mer observations	Colored proportion
</xsl:text>

<xsl:for-each select="root/entry">
	<xsl:sort data-type="number" order="descending" select="recursive/assembledKmerObservations" />
	<xsl:if test="taxon/rank/text() = 'species'">
	<xsl:text>species	</xsl:text>
		<xsl:value-of select="taxon/name"/>
		<xsl:text>	</xsl:text>
		<xsl:value-of select="recursive/assembledKmerObservations"/>
		<xsl:text>	</xsl:text>
		<xsl:value-of select=" recursive/coloredProportionInRank"/>
		<xsl:text>
</xsl:text>
	</xsl:if>
</xsl:for-each>
</xsl:template>
</xsl:stylesheet> 
